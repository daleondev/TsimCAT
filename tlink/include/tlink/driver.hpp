#pragma once
#include <string_view>
#include <vector>
#include <cstddef>
#include <memory>
#include <span>
#include <any>
#include <chrono>
#include "core/task.hpp"
#include "core/result.hpp"
#include "core/channel.hpp"

namespace tlink
{

    // Standard data packet for subscriptions
    using RawDataPacket = std::vector<std::byte>;

    // The stream type used by subscribers
    using RawDataStream = AsyncChannel<Result<RawDataPacket>>;

    enum class SubscriptionType
    {
        OnChange, // Trigger only when value changes
        Cyclic    // Trigger at fixed intervals
    };

    struct RawSubscription
    {
        uint64_t id;
        RawDataStream stream;
        RawSubscription(uint64_t i) : id{i}, stream{} {}
    };

    /**
     * @brief Abstract interface for a protocol driver.
     * A driver handles a single connection and its data exchange.
     */
    class IDriver
    {
    public:
        virtual ~IDriver() = default;

        /**
         * @brief Establishes the connection to the remote device.
         * @param timeout Connection timeout. 0 means infinite.
         */
        virtual auto connect(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<void>> = 0;

        /**
         * @brief Closes the connection.
         */
        virtual auto disconnect(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<void>> = 0;

        /**
         * @brief Read into a generic buffer.
         */
        virtual auto readInto(std::string_view path, std::span<std::byte> dest, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<size_t>> = 0;

        /**
         * @brief Low-level write operation.
         */
        virtual auto writeFrom(std::string_view path, std::span<const std::byte> src, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<void>> = 0;

        /**
         * @brief Subscribe to value changes.
         * Returns a channel that can be co_awaited to receive updates.
         * The driver pushes updates into this channel.
         *
         * @param path Protocol-specific path.
         * @param type The trigger type (OnChange or Cyclic).
         * @param interval The cycle or sampling interval (usage depends on driver).
         * @return A shared pointer to the data stream.
         */
        virtual auto subscribe(std::string_view path, SubscriptionType type = SubscriptionType::OnChange, std::chrono::milliseconds interval = std::chrono::milliseconds(0)) -> Task<Result<std::shared_ptr<RawSubscription>>> = 0;

        virtual auto unsubscribe(std::shared_ptr<RawSubscription> subscription) -> Task<Result<void>> = 0;

        /**
         * @brief Low-level read operation.
         */
        auto readRaw(std::string_view path, size_t maxSize = 1024, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<std::vector<std::byte>>>
        {
            std::vector<std::byte> data(maxSize);
            auto result = co_await readInto(path, data, timeout);
            if (!result)
            {
                co_return std::unexpected(result.error());
            }
            data.resize(result.value());
            co_return data;
        }

        /**
         * @brief Low-level write operation.
         */
        auto writeRaw(std::string_view path, const std::vector<std::byte> &data, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<void>>
        {
            co_return co_await writeFrom(path, data, timeout);
        }

        template <typename T>
        auto read(std::string_view path, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<T>>
        {
            T value{};
            auto result = co_await readInto(path, std::as_writable_bytes(std::span{&value, 1}), timeout);
            if (!result)
            {
                co_return std::unexpected(result.error());
            }
            co_return value;
        }

        auto write(std::string_view path, const auto &value, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) -> Task<Result<void>>
        {
            co_return co_await writeFrom(path, std::as_bytes(std::span{&value, 1}), timeout);
        }
    };

} // namespace tlink
