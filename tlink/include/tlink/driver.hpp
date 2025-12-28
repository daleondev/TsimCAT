#pragma once
#include <string_view>
#include <vector>
#include <cstddef>
#include "core/task.hpp"
#include "core/result.hpp"

namespace tlink {

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
    virtual Task<Result<void>> connect() = 0;

    /**
     * @brief Closes the connection.
     */
    virtual Task<Result<void>> disconnect() = 0;

    /**
     * @brief Low-level read operation.
     * @param path Protocol-specific path (e.g., "Main.Var" for ADS, "ns=2;i=1" for OPCUA).
     */
    virtual Task<Result<std::vector<std::byte>>> read_raw(std::string_view path) = 0;

    /**
     * @brief Low-level write operation.
     */
    virtual Task<Result<void>> write_raw(std::string_view path, const std::vector<std::byte>& data) = 0;
};

} // namespace tlink
