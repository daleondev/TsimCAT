#pragma once

#include <concepts>
#include <mutex>

namespace tlink::coro::utils
{
    namespace detail
    {
        template<typename T>
        concept Queue = requires(T t, typename T::value_type v) {
            typename T::value_type;

            { t.front() } -> std::convertible_to<typename T::value_type&>;

            requires(requires {
                { t.pop() } -> std::same_as<void>;
            } || requires {
                { t.pop_front() } -> std::same_as<void>;
            });

            requires(requires {
                { t.push(v) } -> std::same_as<void>;
            } || requires {
                { t.push_back(v) } -> std::same_as<void>;
            });
        };

        template<typename T>
        concept Lockable = requires(T t) {
            { t.lock() };
            { t.unlock() };
        };

        struct NotLocked
        {
            auto lock() -> void {}
            auto unlock() -> void {}
        };
    }

    template<detail::Lockable L = detail::NotLocked>
    auto pop(detail::Queue auto& queue, L&& lockable = detail::NotLocked{})
    {
        std::scoped_lock lock(lockable);

        auto val{ std::move(queue.front()) };
        if constexpr (requires { queue.pop(); }) {
            queue.pop();
        }
        else {
            queue.pop_front();
        }
        return val;
    }

    template<detail::Lockable L = detail::NotLocked>
    auto push(detail::Queue auto& queue, auto value, L&& lockable = detail::NotLocked{}) -> void
    {
        std::scoped_lock lock(lockable);

        if constexpr (requires { queue.push(value); }) {
            queue.push(std::move(value));
        }
        else {
            queue.push_back(std::move(value));
        }
    }
}