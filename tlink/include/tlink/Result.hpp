#pragma once

#include <expected>
#include <system_error>

namespace tlink
{
    template<typename T>
    using Result = std::expected<T, std::error_code>;

    inline auto success() -> Result<void> { return {}; }
} // namespace tlink
