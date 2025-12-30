#pragma once

#include "Context.hpp"
#include "Task.hpp"

#include <cstring>
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
            m_queue.push_back(std::move(raw));

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

        auto next() -> detail::RawAsyncAwaiter;

      protected:
        std::mutex m_mutex;
        std::deque<Bytes> m_queue;
        struct
        {
            std::coroutine_handle<> handle;
            IExecutor* executor;
        } m_coro{ nullptr, nullptr };

        friend class detail::RawAsyncAwaiter;
    };

    template<typename T>
    class AsyncChannel
    {
      public:
        AsyncChannel(RawAsyncChannel& rawChannel)
          : m_rawChannel{ rawChannel }
        {
        }

        auto push(T value) -> void { m_rawChannel.push(std::as_bytes(std::span{ &value, 1 })); }

        auto next() -> Task<T>
        {
            auto raw = co_await m_rawChannel.next();
            T val{};
            std::memcpy(&val, raw.data(), sizeof(T));
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

            auto await_resume() -> RawAsyncChannel::Bytes
            {
                std::lock_guard lock(channel.m_mutex);
                auto raw{ std::move(channel.m_queue.front()) };
                channel.m_queue.pop_front();
                return raw;
            }
        };
    }

    inline auto RawAsyncChannel::next() -> detail::RawAsyncAwaiter { return { *this }; }
}