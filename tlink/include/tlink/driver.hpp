#pragma once

#include "Result.hpp"
#include "coroutine/coroutine.hpp"

#include <any>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace tlink
{
    static constexpr std::chrono::milliseconds NO_TIMEOUT{ std::chrono::milliseconds(0) };

    class IDriver;

    struct RawSubscription
    {
        const uint64_t id;
        coro::RawAsyncChannel stream;

        explicit RawSubscription(uint64_t i)
          : id(i)
        {
        }
    };

    enum class SubscriptionType
    {
        OnChange,
        Cyclic
    };

    template<typename T>
    class Subscription;

    class IDriver
    {
      public:
        virtual ~IDriver() = default;

        virtual auto connect(std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> = 0;
        virtual auto disconnect(std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> = 0;

        virtual auto readInto(std::string_view path,
                              std::span<std::byte> dest,
                              std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<size_t>> = 0;

        virtual auto writeFrom(std::string_view path,
                               std::span<const std::byte> src,
                               std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> = 0;

        virtual auto subscribeRaw(std::string_view path,
                                  SubscriptionType type = SubscriptionType::OnChange,
                                  std::chrono::milliseconds interval = NO_TIMEOUT)
          -> coro::Task<Result<std::shared_ptr<RawSubscription>>> = 0;

        virtual auto unsubscribeRaw(std::shared_ptr<RawSubscription> subscription) -> coro::Task<Result<void>> = 0;

        // Synchronous cleanup for deleter
        virtual auto unsubscribeRawSync(uint64_t id) -> void = 0;

        template<typename T>
        auto read(std::string_view path, std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<T>>
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

        template<typename T>
        auto subscribe(std::string_view path,
                       SubscriptionType type = SubscriptionType::OnChange,
                       std::chrono::milliseconds interval = NO_TIMEOUT) -> coro::Task<Result<Subscription<T>>>;

        template<typename T>
        auto unsubscribe(Subscription<T>& sub) -> coro::Task<Result<void>>;
    };

    template<typename T>
    class Subscription
    {
      public:
        Subscription() = default;
        Subscription(std::shared_ptr<RawSubscription> raw)
          : m_raw(std::move(raw))
        {
            if (m_raw) {
                stream = coro::AsyncChannel<T>(m_raw->stream);
            }
        }

        ~Subscription() = default;

        Subscription(const Subscription&) = default;
        Subscription& operator=(const Subscription&) = default;
        Subscription(Subscription&&) noexcept = default;
        Subscription& operator=(Subscription&&) noexcept = default;

        [[nodiscard]] auto isValid() const noexcept -> bool { return m_raw != nullptr; }
        [[nodiscard]] auto id() const noexcept -> uint64_t { return m_raw ? m_raw->id : 0; }

        auto getRaw() const -> std::shared_ptr<RawSubscription> { return m_raw; }

        coro::AsyncChannel<T> stream;

      private:
        std::shared_ptr<RawSubscription> m_raw;
    };

    template<typename T>
    auto IDriver::subscribe(std::string_view path,
                           SubscriptionType type,
                           std::chrono::milliseconds interval) -> coro::Task<Result<Subscription<T>>>
    {
        auto rawSub = co_await subscribeRaw(path, type, interval);
        if (!rawSub) {
            co_return std::unexpected(rawSub.error());
        }

        co_return Subscription<T>{ rawSub.value() };
    }

    template<typename T>
    auto IDriver::unsubscribe(Subscription<T>& sub) -> coro::Task<Result<void>>
    {
        auto raw = sub.getRaw();
        if (!raw)
            co_return success();
        co_return co_await unsubscribeRaw(raw);
    }

} // namespace tlink