#pragma once

#include "Context.hpp"

#include <utility>

namespace tlink::coro
{
    namespace detail
    {
        template<typename T>
        struct AsyncAwaiter;
    }

    template<typename T>
    class AsyncChannel
    {
      public:
        auto push(T value) -> void
        {
            std::unique_lock lock(m_mutex);
            m_queue.push_back(std::move(value));

            if (m_coro.handle) {
                auto [handle, exec] = std::exchange(m_coro, { nullptr, nullptr });
                lock.unlock();
                if (exec) {
                    exec->schedule(handle);
                }
                else {
                    handle.resume();
                }
            };
        }

        auto next() -> detail::AsyncAwaiter<T> { return { *this }; }

      private:
        std::mutex m_mutex;
        std::deque<T> m_queue;
        struct
        {
            std::coroutine_handle<> handle;
            IExecutor* executor;
        } m_coro{ nullptr, nullptr };

        template<typename U>
        friend class detail::AsyncAwaiter;
    };

    namespace detail
    {
        template<typename T>
        struct AsyncAwaiter
        {
            AsyncChannel<T>& channel;

            auto await_ready() -> bool
            {
                std::lock_guard lock(channel.m_mutex);
                return !channel.m_queue.empty();
            }

            template<typename P>
            auto await_suspend(std::coroutine_handle<P> handle) -> bool
            {
                std::lock_guard lock(channel.m_mutex);

                if (!channel.m_queue.empty()) {
                    return false;
                }

                channel.m_coro.handle = handle;
                if constexpr (requires { handle.promise().executor; }) {
                    channel.m_coro.executor = handle.promise().executor;
                }
                else {
                    channel.m_coro.executor = nullptr;
                }

                return true;
            }

            auto await_resume() -> T
            {
                std::lock_guard lock(channel.m_mutex);
                T val{ std::move(channel.m_queue.front()) };
                channel.m_queue.pop_front();
                return val;
            }
        };
    }
}