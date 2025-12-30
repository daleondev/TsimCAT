#pragma once

#include "Result.hpp"
#include "coroutine/coroutine.hpp"

#include <any>
#include <chrono>
#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace tlink
{
    static constexpr std::chrono::milliseconds NO_TIMEOUT{ std::chrono::milliseconds(0) };

    struct RawSubscription
    {
        const uint64_t id;
        coro::RawAsyncChannel stream;
    };

    template<typename T>
    struct Subscription
    {
        std::shared_ptr<RawSubscription> rawSub;
        coro::AsyncChannel<T> stream;
    };

    enum class SubscriptionType
    {
        OnChange,
        Cyclic
    };

    class IDriver
    {
      public:
        virtual ~IDriver() = default;

        // clang-format off
        virtual auto connect(std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> = 0;
        virtual auto disconnect(std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> = 0;

        virtual auto readInto(std::string_view path,
                              std::span<std::byte> dest,
                              std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<size_t>> = 0;

        virtual auto writeFrom(std::string_view path,
                               std::span<const std::byte> src,
                               std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> = 0;

        virtual auto subscribeRaw(std::string_view path, SubscriptionType type = SubscriptionType::OnChange,
                               std::chrono::milliseconds interval = NO_TIMEOUT) -> coro::Task<Result<std::shared_ptr<RawSubscription>>> = 0;
        virtual auto unsubscribe(std::shared_ptr<RawSubscription> subscription) -> coro::Task<Result<void>> = 0;
        // clang-format on

        template<typename T>
        auto read(std::string_view path, std::chrono::milliseconds timeout = NO_TIMEOUT)
          -> coro::Task<Result<T>>
        {
            T value{};
            auto result = co_await readInto(path, std::as_writable_bytes(std::span{ &value, 1 }), timeout);
            if (!result) {
                co_return std::unexpected(result.error());
            }
            co_return value;
        }

        auto write(std::string_view path, const auto& value, std::chrono::milliseconds timeout = NO_TIMEOUT)
          -> coro::Task<Result<void>>
        {
            co_return co_await writeFrom(path, std::as_bytes(std::span{ &value, 1 }), timeout);
        }

        auto subscribe(std::string_view path,
                       SubscriptionType type = SubscriptionType::OnChange,
                       std::chrono::milliseconds interval = NO_TIMEOUT)
          -> coro::Task<Result<Subscription<int>>>
        {
            auto rawSub = co_await subscribeRaw(path, type, interval);
            if (!rawSub) {
                co_return std::unexpected(rawSub.error());
            }

            co_return Subscription<int>{ .rawSub = rawSub.value(),
                                         .stream = coro::AsyncChannel<int>(rawSub.value()->stream) };
        };
    };

} // namespace tlink
