#pragma once

#include "../IDriver.hpp"
#include "Coroutines/Task.hpp"
#include <span>
#include <string_view>

namespace core::comm
{
    class ISymbolicComm : public IDriver
    {
      public:
        virtual ~ISymbolicComm() = default;

        virtual auto readRaw(std::string_view symbol, std::span<std::byte> dest)
          -> coro::Task<result::Result<size_t>> = 0;
        virtual auto writeRaw(std::string_view symbol, std::span<const std::byte> src)
          -> coro::Task<result::Result<void>> = 0;

        template<typename T>
        auto read(std::string_view symbol) -> coro::Task<result::Result<T>>
        {
            T value{};
            auto result = co_await readRaw(symbol, std::as_writable_bytes(std::span{ &value, 1 }));
            if (!result)
                return std::unexpected(result.error());
            co_return value;
        }

        template<typename T>
        auto write(std::string_view symbol, const T& value) -> coro::Task<result::Result<void>>
        {
            return writeRaw(symbol, std::as_bytes(std::span{ &value, 1 }));
        }
    };
}
