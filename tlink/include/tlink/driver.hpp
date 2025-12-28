#pragma once
#include <string_view>
#include <vector>
#include <cstddef>
#include <memory>
#include "core/task.hpp"
#include "core/result.hpp"
#include "core/channel.hpp"

namespace tlink {

// Standard data packet for subscriptions
using DataPacket = std::vector<std::byte>;

// The stream type used by subscribers
using DataStream = AsyncChannel<Result<DataPacket>>;

/**
 * @brief Abstract interface for a protocol driver.
 * A driver handles a single connection and its data exchange.
 */
class IDriver {
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
     * @brief Low-level read operation.
     */
    virtual auto read_raw(std::string_view path) -> Task<Result<std::vector<std::byte>>> = 0;

    /**
     * @brief Low-level write operation.
     */
    virtual auto write_raw(std::string_view path, const std::vector<std::byte>& data) -> Task<Result<void>> = 0;

    /**
     * @brief Subscribe to value changes.
     * Returns a channel that can be co_awaited to receive updates.
     * The driver pushes updates into this channel.
     * 
     * @param path Protocol-specific path.
     * @return A shared pointer to the data stream.
     */
    virtual auto subscribe(std::string_view path) -> Task<Result<std::shared_ptr<DataStream>>> = 0;
};

} // namespace tlink
