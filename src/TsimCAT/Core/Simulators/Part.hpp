#pragma once

#include <cstdint>

namespace core::sim
{
    struct Part
    {
        uint32_t id;
        uint8_t type;
        double position; // Position along the conveyor length (mm)
        double width;    // Width of the part (mm)
        double length;   // Length of the part (mm)
        double height;   // Height of the part (mm)
    };
}
