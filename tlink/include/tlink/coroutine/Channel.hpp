#pragma once

#include "Context.hpp"

#include <cstring>
#include <span>
#include <utility>
#include <vector>

namespace tlink::coro
{
    namespace detail
    {
        template<typename T>
        struct AsyncAwaiter;
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

        auto next() -> detail::AsyncAwaiter<Bytes>;

      protected:
        std::mutex m_mutex;
        std::deque<Bytes> m_queue;
        struct
        {
            std::coroutine_handle<> handle;
            IExecutor* executor;
        } m_coro{ nullptr, nullptr };

        template<typename U>
        friend class detail::AsyncAwaiter;
    };

    template<typename T>
    class AsyncChannel : public RawAsyncChannel
    {
      public:
        auto push(T value) -> void { RawAsyncChannel::push(std::as_bytes(std::span{ &value, 1 })); }
        auto next() -> detail::AsyncAwaiter<T>;
    };

    namespace detail
    {
        template<typename T>
        struct AsyncAwaiter
        {
            // static_assert(std::is_same_v<T, RawAsyncChannel::Bytes> || std::is_trivially_copyable_v<T>);

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

            auto await_resume() -> T
            {
                RawAsyncChannel::Bytes raw;
                {
                    std::lock_guard lock(channel.m_mutex);
                    raw = std::move(channel.m_queue.front());
                    channel.m_queue.pop_front();
                }

                if constexpr (std::is_same_v<T, RawAsyncChannel::Bytes>) {
                    return raw;
                }
                else {
                    T val{};
                    std::memcpy(&val, raw.data(), sizeof(T));
                    return val;
                }
            }
        };
    }

    inline auto RawAsyncChannel::next() -> detail::AsyncAwaiter<Bytes> { return { *this }; }
    template<typename T>
    auto AsyncChannel<T>::next() -> detail::AsyncAwaiter<T>
    {
        return { *this };
    }
}