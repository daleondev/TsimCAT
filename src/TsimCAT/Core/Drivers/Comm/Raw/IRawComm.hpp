#pragma once

#include "../IDriver.hpp"
#include "Coroutines/Task.hpp"
#include <span>

namespace core::comm
{
    class IRawComm : public IDriver
    {
      public:
        virtual ~IRawComm() = default;

        virtual auto send(std::span<const std::byte> data) -> coro::Task<result::Result<void>> = 0;
        virtual auto receive(std::span<std::byte> dest) -> coro::Task<result::Result<size_t>> = 0;
    };
}
