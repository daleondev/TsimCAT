#pragma once

#include "Context.hpp"
#include "Task.hpp"

#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace tlink::coro
{
    namespace detail
    {
        struct RawAsyncAwaiter;
    }

    enum class ChannelMode
    {
        Broadcast,   // Every waiter gets every update (Pub/Sub)
        LoadBalancer // Updates are distributed among waiters (Work Queue)
    };

    class RawAsyncChannel
    {
      public:
        using Bytes = std::vector<std::byte>;

      protected:
        struct Waiter
        {
            std::coroutine_handle<> handle;
            IExecutor* executor;
            // For broadcast, we need a place to put the result
            std::optional<Bytes>* resultDest = nullptr;
        };

        struct State
        {
            std::mutex mutex;
            std::deque<Bytes> queue; // Used for LoadBalancer or buffering when no waiters
            bool closed = false;
            std::deque<Waiter> waiters;
            ChannelMode mode = ChannelMode::Broadcast;
        };

      public:
        RawAsyncChannel()
          : m_state(std::make_shared<State>())
        {
        }

        auto setMode(ChannelMode mode) -> void
        {
            std::lock_guard lock(m_state->mutex);
            m_state->mode = mode;
        }

        auto push(Bytes raw) -> void
        {
            std::unique_lock lock(m_state->mutex);
            if (m_state->closed) {
                return;
            }

            if (m_state->waiters.empty()) {
                m_state->queue.push_back(std::move(raw));
                return;
            }

            if (m_state->mode == ChannelMode::LoadBalancer) {
                auto waiter = m_state->waiters.front();
                m_state->waiters.pop_front();
                
                if (waiter.resultDest) {
                    *waiter.resultDest = std::move(raw);
                }

                lock.unlock();
                if (waiter.executor) {
                    waiter.executor->schedule(waiter.handle);
                } else {
                    waiter.handle.resume();
                }
            }
            else {
                // Broadcast: Every current waiter gets a copy
                auto toResume = std::move(m_state->waiters);
                m_state->waiters.clear();
                
                for (auto& waiter : toResume) {
                    if (waiter.resultDest) {
                        *waiter.resultDest = raw; // Copy
                    }
                }

                lock.unlock();
                for (auto& waiter : toResume) {
                    if (waiter.executor) {
                        waiter.executor->schedule(waiter.handle);
                    } else {
                        waiter.handle.resume();
                    }
                }
            }
        }

        auto close() -> void
        {
            std::deque<Waiter> toResume;
            {
                std::lock_guard lock(m_state->mutex);
                if (m_state->closed) {
                    return;
                }
                m_state->closed = true;
                toResume = std::move(m_state->waiters);
            }

            for (auto& waiter : toResume) {
                if (waiter.executor) {
                    waiter.executor->schedule(waiter.handle);
                }
                else {
                    waiter.handle.resume();
                }
            }
        }

        auto next(std::optional<Bytes>& dest) -> detail::RawAsyncAwaiter;

      protected:
        std::shared_ptr<State> m_state;

        friend class detail::RawAsyncAwaiter;
        template<typename T>
            requires std::is_trivially_copyable_v<T>
        friend class AsyncChannel;
    };

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    class AsyncChannel
    {
      public:
        AsyncChannel() = default;
        AsyncChannel(const RawAsyncChannel& raw)
          : m_state(raw.m_state)
        {
        }

        auto setMode(ChannelMode mode) -> void
        {
            if (m_state) {
                std::lock_guard lock(m_state->mutex);
                m_state->mode = mode;
            }
        }

        auto next() -> Task<std::optional<T>>
        {
            RawAsyncChannel handle;
            handle.m_state = m_state;
            
            std::optional<RawAsyncChannel::Bytes> result;
            auto awaiter = handle.next(result);
            co_await awaiter;

            if (!result) {
                co_return std::nullopt;
            }
            T val{};
            std::memcpy(&val, result->data(), sizeof(T));
            co_return val;
        }

      private:
        std::shared_ptr<RawAsyncChannel::State> m_state;
    };

    namespace detail
    {
        struct RawAsyncAwaiter
        {
            std::shared_ptr<RawAsyncChannel::State> state;
            std::optional<RawAsyncChannel::Bytes>& dest;

            auto await_ready() -> bool
            {
                std::lock_guard lock(state->mutex);
                if (state->closed) return true;
                
                // If there's something in the queue (from buffered push), take it
                if (!state->queue.empty()) {
                    dest = std::move(state->queue.front());
                    state->queue.pop_front();
                    return true;
                }
                return false;
            }

            template<typename P>
            auto await_suspend(std::coroutine_handle<P> handle) -> bool
            {
                std::lock_guard lock(state->mutex);

                // Re-check state under lock
                if (state->closed || !state->queue.empty()) {
                    return false;
                }

                IExecutor* executor = nullptr;
                if constexpr (requires { handle.promise().executor; }) {
                    executor = handle.promise().executor;
                }

                state->waiters.push_back({ handle, executor, &dest });
                return true;
            }

            auto await_resume() -> void
            {
                // Result is already in 'dest' or dest is nullopt (closed)
            }
        };
    }

    inline auto RawAsyncChannel::next(std::optional<Bytes>& dest) -> detail::RawAsyncAwaiter 
    { 
        return { m_state, dest }; 
    }
}