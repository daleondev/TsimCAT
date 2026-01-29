#pragma once

#include "Common/Result.hpp"
#include "Coroutines/Task.hpp"

#include <chrono>

namespace core::link
{
    static constexpr std::chrono::milliseconds NO_TIMEOUT{ std::chrono::milliseconds(0) };

    enum class Status
    {
        Disconnected,
        Connecting,
        Connected,
        Faulty
    };

    class ILink
    {
      public:
        virtual ~ILink() = default;

        virtual auto connect(std::chrono::milliseconds timeout = NO_TIMEOUT)
          -> coro::Task<result::Result<void>> = 0;
        virtual auto disconnect() -> coro::Task<result::Result<void>> = 0;
        virtual auto status() const -> Status = 0;
    };
}
