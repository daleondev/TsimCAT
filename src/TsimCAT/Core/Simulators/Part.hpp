#pragma once

#include <cstdint>

namespace core::sim
{
    struct Part
    {
        uint32_t id{ 0 };
        uint8_t type{ 0 };
        double position{ 0.0 }; // Position along the conveyor length (mm)
        double width{ 0.0 };    // Width of the part (mm)
        double length{ 0.0 };   // Length of the part (mm)
        double height{ 0.0 };   // Height of the part (mm)
    };
}
