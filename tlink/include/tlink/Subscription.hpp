#pragma once

#include "coroutine/Channel.hpp"

namespace tlink
{
    enum class SubscriptionType
    {
        OnChange,
        Cyclic
    };

    struct RawSubscription
    {
        const uint64_t id;
        coro::RawAsyncChannel stream;
        RawSubscription(uint64_t i)
          : id(i)
        {
        }
    };

    template<typename T>
    struct Subscription
    {
        Subscription() = default;
        Subscription(std::shared_ptr<RawSubscription> sub)
          : raw(std::move(sub))
        {
            if (raw) {
                stream = coro::AsyncChannel<T>(raw->stream);
            }
        }

        auto isValid() const noexcept -> bool { return raw != nullptr; }
        auto id() const noexcept -> uint64_t { return raw ? raw->id : 0; }

        coro::AsyncChannel<T> stream;
        std::shared_ptr<RawSubscription> raw;
    };
} // namespace tlink