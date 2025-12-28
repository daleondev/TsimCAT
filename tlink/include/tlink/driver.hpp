#pragma once
#include <string_view>
#include <vector>
#include <cstddef>
#include <memory>
#include <span>
#include "core/task.hpp"
#include "core/result.hpp"
#include "core/channel.hpp"

namespace tlink
{

    // Standard data packet for subscriptions
    using DataPacket = std::vector<std::byte>;

    // The stream type used by subscribers
    using DataStream = AsyncChannel<Result<DataPacket>>;

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
         */
        virtual auto connect() -> Task<Result<void>> = 0;

        /**
         * @brief Closes the connection.
         */
        virtual auto disconnect() -> Task<Result<void>> = 0;

        /**
         * @brief Read into a generic buffer.
         */
        virtual auto readInto(std::string_view path, std::span<std::byte> dest) -> Task<Result<size_t>> = 0;

        /**
         * @brief Low-level write operation.
         */
        virtual auto writeFrom(std::string_view path, std::span<const std::byte> src) -> Task<Result<void>> = 0;

        /**
         * @brief Subscribe to value changes.
         * Returns a channel that can be co_awaited to receive updates.
         * The driver pushes updates into this channel.
         *
         * @param path Protocol-specific path.
         * @return A shared pointer to the data stream.
         */
        virtual auto subscribe(std::string_view path) -> Task<Result<std::shared_ptr<DataStream>>> = 0;

        virtual auto unsubscribe(std::string_view path) -> Task<Result<void>> = 0;

        /**
         * @brief Low-level read operation.
         */
        auto readRaw(std::string_view path, size_t maxSize = 1024) -> Task<Result<std::vector<std::byte>>>
        {
            std::vector<std::byte> data(maxSize);
            auto result = co_await readInto(path, data);
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
        auto writeRaw(std::string_view path, const std::vector<std::byte> &data) -> Task<Result<void>>
        {
            co_return co_await writeFrom(path, data);
        }

        template <typename T>
        auto read(std::string_view path) -> Task<Result<T>>
        {
            T value{};
            auto result = co_await readInto(path, std::as_writable_bytes(std::span{&value, 1}));
            if (!result)
            {
                co_return std::unexpected(result.error());
            }
            co_return value;
        }

        auto read(std::string_view path, const auto &value) -> Task<Result<void>>
        {
            co_return co_await writeFrom(path, std::as_bytes(std::span{&value, 1}));
        }
    };

} // namespace tlink
