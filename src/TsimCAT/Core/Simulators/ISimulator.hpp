#pragma once

#include "../Coroutines/Task.hpp"
#include "../Common/Result.hpp"
#include <string>

namespace core::sim
{
    class ISimulator
    {
    public:
        virtual ~ISimulator() = default;

        virtual auto name() const -> std::string = 0;
        
        virtual auto initialize() -> coro::Task<result::Result<void>> = 0;
        virtual auto start() -> void = 0;
        virtual auto stop() -> void = 0;
        
        virtual auto update(double deltaTimeSeconds) -> void = 0;
    };
}
