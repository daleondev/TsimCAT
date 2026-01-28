#pragma once

#include "../Coroutines/Task.hpp"
#include <string>

namespace core::sim
{
    class ISimulator
    {
    public:
        virtual ~ISimulator() = default;

        virtual auto name() const -> std::string = 0;
        
        virtual auto initialize() -> coro::Task<void> = 0;
        virtual auto start() -> void = 0;
        virtual auto stop() -> void = 0;
        
        virtual auto update(double deltaTimeSeconds) -> void = 0;
    };
}
