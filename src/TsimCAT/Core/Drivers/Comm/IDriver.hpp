#pragma once

#include "../../Coroutines/Task.hpp"
#include <chrono>
#include <expected>
#include <system_error>

namespace core::comm
{
    using ResultVoid = std::expected<void, std::error_code>;

    enum class Status
    {
        Disconnected,
        Connecting,
        Connected,
        Faulty
    };

    class IDriver
    {
      public:
        virtual ~IDriver() = default;

        virtual auto connect(std::chrono::milliseconds timeout = std::chrono::milliseconds(0))
          -> coro::Task<ResultVoid> = 0;
        virtual auto disconnect() -> coro::Task<ResultVoid> = 0;
        virtual auto status() const -> Status = 0;
    };
}
