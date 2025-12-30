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

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    class AsyncChannel;

    class RawAsyncChannel
    {
      public:
        using Bytes = std::vector<std::byte>;

      protected:
        struct Waiter
        {
            std::coroutine_handle<> handle;
            IExecutor* executor;
        };

        struct State
        {
            std::mutex mutex;
            std::deque<Bytes> queue;
            bool closed = false;
            std::deque<Waiter> waiters;
        };

      public:
        RawAsyncChannel()
          : m_state(std::make_shared<State>())
        {
        }

        auto push(Bytes raw) -> void
        {
            std::unique_lock lock(m_state->mutex);
            if (m_state->closed) {
                return;
            }
            m_state->queue.push_back(std::move(raw));

            if (!m_state->waiters.empty()) {
                auto [handle, exec] = m_state->waiters.front();
                m_state->waiters.pop_front();
                lock.unlock();
                if (exec) {
                    exec->schedule(handle);
                }
                else {
                    handle.resume();
                }
            };
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

            for (auto& [handle, exec] : toResume) {
                if (exec) {
                    exec->schedule(handle);
                }
                else {
                    handle.resume();
                }
            }
        }

        auto next() -> detail::RawAsyncAwaiter;

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

        auto push(T value) -> void
        {
            RawAsyncChannel handle;
            handle.m_state = m_state;
            handle.push(std::vector<std::byte>(
              reinterpret_cast<const std::byte*>(&value), reinterpret_cast<const std::byte*>(&value) + sizeof(T)));
        }

        auto next() -> Task<std::optional<T>>
        {
            RawAsyncChannel handle;
            handle.m_state = m_state;
            auto raw = co_await handle.next();
            if (!raw) {
                co_return std::nullopt;
            }
            T val{};
            std::memcpy(&val, raw->data(), sizeof(T));
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

            auto await_ready() -> bool
            {
                std::lock_guard lock(state->mutex);
                return !state->queue.empty() || state->closed;
            }

            template<typename P>
            auto await_suspend(std::coroutine_handle<P> handle) -> bool
            {
                std::lock_guard lock(state->mutex);

                if (!state->queue.empty() || state->closed) {
                    return false;
                }

                IExecutor* executor = nullptr;
                if constexpr (requires { handle.promise().executor; }) {
                    executor = handle.promise().executor;
                }

                state->waiters.push_back({ handle, executor });
                return true;
            }

            auto await_resume() -> std::optional<RawAsyncChannel::Bytes>
            {
                std::lock_guard lock(state->mutex);
                if (state->queue.empty()) {
                    return std::nullopt;
                }

                auto raw{ std::move(state->queue.front()) };
                state->queue.pop_front();
                return raw;
            }
        };
    }

    inline auto RawAsyncChannel::next() -> detail::RawAsyncAwaiter { return { m_state }; }
}