#pragma once

#include "coroutine/Task.hpp"

#include "Result.hpp"
#include "Subscription.hpp"

#include <chrono>

namespace tlink
{
    static constexpr std::chrono::milliseconds NO_TIMEOUT{ std::chrono::milliseconds(0) };

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

        virtual auto subscribeRaw(std::string_view path,
                                  size_t size,
                                  SubscriptionType type = SubscriptionType::OnChange,
                                  std::chrono::milliseconds interval = NO_TIMEOUT) -> coro::Task<Result<std::shared_ptr<RawSubscription>>> = 0;
        virtual auto unsubscribeRaw(std::shared_ptr<RawSubscription> subscription) -> coro::Task<Result<void>> = 0;
        virtual auto unsubscribeRawSync(uint64_t id) -> void = 0;
        // clang-format on

        template<typename T>
        auto read(std::string_view path, std::chrono::milliseconds timeout = NO_TIMEOUT)
          -> coro::Task<Result<T>>
        {
            T value{};
            auto result{ co_await readInto(path, std::as_writable_bytes(std::span{ &value, 1 }), timeout) };
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

        template<typename T>
        auto subscribe(std::string_view path,
                       SubscriptionType type = SubscriptionType::OnChange,
                       std::chrono::milliseconds interval = NO_TIMEOUT) -> coro::Task<Result<Subscription<T>>>
        {
            auto rawSub{ co_await subscribeRaw(path, sizeof(T), type, interval) };
            if (!rawSub) {
                co_return std::unexpected(rawSub.error());
            }

            co_return Subscription<T>{ rawSub.value() };
        }

        template<typename T>
        auto unsubscribe(Subscription<T>& sub) -> coro::Task<Result<void>>
        {
            if (!sub.raw) {
                co_return success();
            }
            co_return co_await unsubscribeRaw(sub.raw);
        }
    };

} // namespace tlink