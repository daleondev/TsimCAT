#pragma once

#include <concepts>
#include <mutex>
#include <ranges>
#include <span>

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

        template<typename T>
        concept Serializable = std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>;
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

    auto memcpy(detail::Serializable auto& dest, const std::ranges::range auto& src) -> bool
    {
        auto destBytes{ std::as_writable_bytes(std::span{ &dest, 1 }) };
        if (src.size() != destBytes.size()) {
            return false;
        }
        std::ranges::copy(src, destBytes.begin());
        return true;
    }

    auto memcpy(std::ranges::range auto& dest, const detail::Serializable auto& src) -> bool
    {
        auto srcBytes{ std::as_bytes(std::span{ &src, 1 }) };
        if (srcBytes.size() != dest.size()) {
            return false;
        }
        std::ranges::copy(srcBytes, dest.begin());
        return true;
    }

    auto memcpy(detail::Serializable auto& dest, const detail::Serializable auto& src) -> bool
    {
        auto srcBytes{ std::as_bytes(std::span{ &src, 1 }) };
        auto destBytes{ std::as_writable_bytes(std::span{ &dest, 1 }) };
        if (srcBytes.size() != destBytes.size()) {
            return false;
        }
        std::ranges::copy(srcBytes, destBytes.begin());
        return true;
    }

    auto memmv(detail::Serializable auto& dest, const std::ranges::range auto& src) -> bool
    {
        auto destBytes{ std::as_writable_bytes(std::span{ &dest, 1 }) };
        if (src.size() != destBytes.size()) {
            return false;
        }
        std::ranges::move(src, destBytes.begin());
        return true;
    }

    auto memmv(std::ranges::range auto& dest, const detail::Serializable auto& src) -> bool
    {
        auto srcBytes{ std::as_bytes(std::span{ &src, 1 }) };
        if (srcBytes.size() != dest.size()) {
            return false;
        }
        std::ranges::move(srcBytes, dest.begin());
        return true;
    }

    auto memmv(detail::Serializable auto& dest, const detail::Serializable auto& src) -> bool
    {
        auto srcBytes{ std::as_bytes(std::span{ &src, 1 }) };
        auto destBytes{ std::as_writable_bytes(std::span{ &dest, 1 }) };
        if (srcBytes.size() != destBytes.size()) {
            return false;
        }
        std::ranges::move(srcBytes, destBytes.begin());
        return true;
    }
}