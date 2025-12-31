#pragma once

#include <cstdint>

namespace tsim::model
{
    /**
     * Commands sent from the PLC to the Conveyor (Digital Interface).
     */
    #pragma pack(push, 1)
    struct ConveyorControl
    {
        uint8_t bRun;          // Enable conveyor movement (1: Run, 0: Stop)
        uint8_t bReverse;      // Set direction (0: Forward, 1: Backward)
    };

    /**
     * Status reported from the Conveyor simulation back to the PLC.
     */
    struct ConveyorStatus
    {
        uint8_t bRunning;      // Is currently moving
        uint8_t bError;        // Fault status
    };
    #pragma pack(pop)
}
