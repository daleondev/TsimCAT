#pragma once

#include "Context.hpp"
#include "Task.hpp"

#include <algorithm>
#include <deque>
#include <list>
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
            std::weak_ptr<void> lifeToken;
            detail::RawAsyncAwaiter* awaiterPtr = nullptr;
        };

        struct State
        {
            std::mutex mutex;
            std::deque<Bytes> queue; // Used for LoadBalancer or buffering when no waiters
            bool closed = false;
            std::list<Waiter> waiters;
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

        auto push(Bytes raw) -> void;
        auto close() -> void;

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

            auto x = result->size();
            if (result->size() != sizeof(T)) {
                co_return std::nullopt;
            }

            T val{};
            auto destBytes{ std::as_writable_bytes(std::span{ &val, 1 }) };
            std::ranges::copy(*result, destBytes.begin());

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
            std::optional<std::list<RawAsyncChannel::Waiter>::iterator> m_iterator;

            ~RawAsyncAwaiter()
            {
                if (m_iterator) {
                    std::lock_guard lock(state->mutex);
                    // Re-check under lock (though single-threaded destruction usually implies safety,
                    // race could happen if 'push' is concurrent with cancellation)
                    if (m_iterator) {
                        state->waiters.erase(*m_iterator);
                    }
                }
            }

            // Called by Channel to signal completion/detachment
            void unlink() { m_iterator = std::nullopt; }

            auto await_ready() -> bool
            {
                std::lock_guard lock(state->mutex);
                if (state->closed)
                    return true;

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
                std::weak_ptr<void> lifeToken;

                if constexpr (requires { handle.promise().executor; }) {
                    executor = handle.promise().executor;
                    if (executor) {
                        lifeToken = executor->getLifeToken();
                    }
                }

                m_iterator = state->waiters.insert(state->waiters.end(),
                                                   { handle, executor, &dest, std::move(lifeToken), this });
                return true;
            }

            auto await_resume() -> void
            {
                // Result is already in 'dest' or dest is nullopt (closed)
                // If we resumed normally, 'unlink()' was already called by 'push'.
            }
        };
    }

    inline auto RawAsyncChannel::next(std::optional<Bytes>& dest) -> detail::RawAsyncAwaiter
    {
        return { m_state, dest };
    }

    inline auto RawAsyncChannel::push(Bytes raw) -> void
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

            // Detach from awaiter so it doesn't try to erase itself on destruction
            if (waiter.awaiterPtr) {
                waiter.awaiterPtr->unlink();
            }

            if (waiter.resultDest) {
                *waiter.resultDest = std::move(raw);
            }

            lock.unlock();
            if (waiter.executor) {
                if (auto token = waiter.lifeToken.lock()) {
                    waiter.executor->schedule(waiter.handle);
                }
            }
            else {
                waiter.handle.resume();
            }
        }
        else {
            // Broadcast: Every current waiter gets a copy
            auto toResume = std::move(m_state->waiters);
            // List moved-from state is valid but unspecified, clear it to be sure
            m_state->waiters.clear();

            for (auto& waiter : toResume) {
                // Detach from awaiter so it doesn't try to erase itself on destruction
                if (waiter.awaiterPtr) {
                    waiter.awaiterPtr->unlink();
                }

                if (waiter.resultDest) {
                    *waiter.resultDest = raw; // Copy
                }
            }

            lock.unlock();
            for (auto& waiter : toResume) {
                if (waiter.executor) {
                    if (auto token = waiter.lifeToken.lock()) {
                        waiter.executor->schedule(waiter.handle);
                    }
                }
                else {
                    waiter.handle.resume();
                }
            }
        }
    }

    inline auto RawAsyncChannel::close() -> void
    {
        std::list<Waiter> toResume;
        {
            std::lock_guard lock(m_state->mutex);
            if (m_state->closed) {
                return;
            }
            m_state->closed = true;
            toResume = std::move(m_state->waiters);
        }

        for (auto& waiter : toResume) {
            if (waiter.awaiterPtr) {
                waiter.awaiterPtr->unlink();
            }

            if (waiter.executor) {
                if (auto token = waiter.lifeToken.lock()) {
                    waiter.executor->schedule(waiter.handle);
                }
            }
            else {
                waiter.handle.resume();
            }
        }
    }
}