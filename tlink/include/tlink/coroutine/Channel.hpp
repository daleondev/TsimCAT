#pragma once

#include "Context.hpp"
#include "Task.hpp"

#include <cstring>
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

    class RawAsyncChannel
    {
      public:
        using Bytes = std::vector<std::byte>;

        auto push(Bytes raw) -> void
        {
            std::unique_lock lock(m_mutex);
            if (m_closed) {
                return;
            }
            m_queue.push_back(std::move(raw));

            if (!m_waiters.empty()) {
                auto [handle, exec] = m_waiters.front();
                m_waiters.pop_front();
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
                std::lock_guard lock(m_mutex);
                m_closed = true;
                toResume = std::move(m_waiters);
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
        struct Waiter
        {
            std::coroutine_handle<> handle;
            IExecutor* executor;
        };

        std::mutex m_mutex;
        std::deque<Bytes> m_queue;
        bool m_closed = false;
        std::deque<Waiter> m_waiters;

        friend class detail::RawAsyncAwaiter;
    };

    template<typename T>
        requires std::is_trivially_copyable_v<T>
    class AsyncChannel
    {
      public:
        AsyncChannel(RawAsyncChannel& rawChannel)
          : m_rawChannel{ rawChannel }
        {
        }

        auto push(T value) -> void { m_rawChannel.push(std::as_bytes(std::span{ &value, 1 })); }

        auto next() -> Task<std::optional<T>>
        {
            auto raw = co_await m_rawChannel.next();
            if (!raw) {
                co_return std::nullopt;
            }
            T val{};
            std::memcpy(&val, raw->data(), sizeof(T));
            co_return val;
        }

      private:
        RawAsyncChannel& m_rawChannel;
    };

    namespace detail
    {
        struct RawAsyncAwaiter
        {
            RawAsyncChannel& channel;

            auto await_ready() -> bool
            {
                std::lock_guard lock(channel.m_mutex);
                return !channel.m_queue.empty() || channel.m_closed;
            }

            template<typename P>
            auto await_suspend(std::coroutine_handle<P> handle) -> bool
            {
                std::lock_guard lock(channel.m_mutex);

                if (!channel.m_queue.empty() || channel.m_closed) {
                    return false;
                }

                IExecutor* executor = nullptr;
                if constexpr (requires { handle.promise().executor; }) {
                    executor = handle.promise().executor;
                }

                channel.m_waiters.push_back({ handle, executor });
                return true;
            }

            auto await_resume() -> std::optional<RawAsyncChannel::Bytes>
            {
                std::lock_guard lock(channel.m_mutex);
                if (channel.m_queue.empty()) {
                    return std::nullopt;
                }

                auto raw{ std::move(channel.m_queue.front()) };
                channel.m_queue.pop_front();
                return raw;
            }
        };
    }

    inline auto RawAsyncChannel::next() -> detail::RawAsyncAwaiter { return { *this }; }
}