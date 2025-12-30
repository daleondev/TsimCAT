#pragma once

#include <expected>
#include <system_error>

namespace tlink
{
    /**
     * @brief Standardized result type for TLink operations.
     * Uses std::expected to provide either the value or an error code.
     */
    template<typename T>
    using Result = std::expected<T, std::error_code>;

    /**
     * @brief Helper for generating success results.
     */
    inline auto success() -> Result<void> { return {}; }

} // namespace tlink
