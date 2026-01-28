#pragma once

#include <string>

namespace core::sim
{
    /**
     * @brief Base interface for simple simulation components (Buttons, Doors, Sensors).
     */
    class IComponent
    {
    public:
        virtual ~IComponent() = default;
        virtual auto name() const -> std::string = 0;
        
        // Components are usually fast and synchronous, 
        // but we keep the option for state updates.
        virtual auto update(double deltaTimeSeconds) -> void = 0;
    };
}
