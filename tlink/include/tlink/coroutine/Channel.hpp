#pragma once

#include "Context.hpp"
#include "Task.hpp"

#include <list>
#include <vector>

namespace tlink::coro
{
    namespace detail
    {
        struct RawAsyncAwaiter;
    }

    enum class ChannelMode
    {
        Broadcast,
        LoadBalancer
    };

    class RawAsyncChannel
    {
      public:
        using Bytes = std::vector<std::byte>;

        auto setMode(ChannelMode mode) -> void
        {
            std::scoped_lock lock(m_state->mutex);
            m_state->mode = mode;
        }

        auto push(Bytes raw) -> void;
        auto close() -> void;

        auto next(std::optional<Bytes>& dest) -> detail::RawAsyncAwaiter;

      protected:
        struct Waiter
        {
            std::coroutine_handle<> handle{};
            IExecutor* executor{ nullptr };
            // For broadcast, we need a place to put the result
            std::optional<Bytes>* resultDest{ nullptr };
            std::weak_ptr<void> lifeToken{};
            detail::RawAsyncAwaiter* awaiterPtr{ nullptr };
        };

        struct State
        {
            std::mutex mutex{};
            std::deque<Bytes> queue{};
            bool closed{ false };
            std::list<Waiter> waiters{};
            ChannelMode mode{ ChannelMode::Broadcast };
        };

        std::shared_ptr<State> m_state{ std::make_shared<State>() };

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
                std::scoped_lock lock(m_state->mutex);
                m_state->mode = mode;
            }
        }

        auto next() -> Task<std::optional<T>>
        {
            RawAsyncChannel raw{};
            raw.m_state = m_state;

            std::optional<RawAsyncChannel::Bytes> result{};
            co_await raw.next(result);

            if (!result || result->size() != sizeof(T)) {
                co_return std::nullopt;
            }

            T val{};
            utils::memcpy(val, result.value());
            co_return val;
        }

      private:
        std::shared_ptr<RawAsyncChannel::State> m_state;
    };

    namespace detail
    {
        struct RawAsyncAwaiter
        {
            std::shared_ptr<RawAsyncChannel::State> state{};
            std::optional<RawAsyncChannel::Bytes>& dest;
            std::optional<std::list<RawAsyncChannel::Waiter>::iterator> m_iterator{};

            ~RawAsyncAwaiter()
            {
                if (m_iterator) {
                    std::scoped_lock lock(state->mutex);
                    if (m_iterator) {
                        state->waiters.erase(*m_iterator);
                    }
                }
            }

            void unlink() { m_iterator = std::nullopt; }

            auto await_ready() -> bool
            {
                std::scoped_lock lock(state->mutex);
                if (state->closed) {
                    return true;
                }

                if (auto raw{ utils::pop(state->queue) }) {
                    dest.emplace(raw.value());
                    return true;
                }
                return false;
            }

            template<typename P>
            auto await_suspend(std::coroutine_handle<P> handle) -> bool
            {
                std::scoped_lock lock(state->mutex);

                if (state->closed || !state->queue.empty()) {
                    return false;
                }

                IExecutor* executor{ nullptr };
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

        static auto resumeWaiter(auto& waiter) -> void
        {
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

    inline auto RawAsyncChannel::next(std::optional<Bytes>& dest) -> detail::RawAsyncAwaiter
    {
        return detail::RawAsyncAwaiter{ m_state, dest };
    }

    inline auto RawAsyncChannel::push(Bytes raw) -> void
    {
        std::unique_lock lock(m_state->mutex);
        if (m_state->closed) {
            return;
        }

        if (m_state->waiters.empty()) {
            // store until new waiter spawns
            m_state->queue.push_back(std::move(raw));
            return;
        }

        if (m_state->mode == ChannelMode::LoadBalancer) {
            auto waiter{ std::move(m_state->waiters.front()) };
            m_state->waiters.pop_front();

            // detach from awaiter so it doesnt try to erase itself on destruction
            if (waiter.awaiterPtr) {
                waiter.awaiterPtr->unlink();
            }

            if (waiter.resultDest) {
                waiter.resultDest->emplace(std::move(raw));
            }

            lock.unlock();
            detail::resumeWaiter(waiter);
        }
        else {
            auto toResume{ std::move(m_state->waiters) };
            m_state->waiters.clear();

            for (auto& waiter : toResume) {
                // detach from awaiter so it doesnt try to erase itself on destruction
                if (waiter.awaiterPtr) {
                    waiter.awaiterPtr->unlink();
                }

                if (waiter.resultDest) {
                    waiter.resultDest->emplace(raw);
                }
            }

            lock.unlock();
            for (auto& waiter : toResume) {
                detail::resumeWaiter(waiter);
            }
        }
    }

    inline auto RawAsyncChannel::close() -> void
    {
        std::list<Waiter> toResume;
        {
            std::scoped_lock lock(m_state->mutex);
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
            detail::resumeWaiter(waiter);
        }
    }
}