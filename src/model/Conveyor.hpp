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
        bool bRun;          // Enable conveyor movement
        bool bReverse;      // Set direction (false: forward, true: backward)
    };

    /**
     * Status reported from the Conveyor simulation back to the PLC.
     */
    struct ConveyorStatus
    {
        bool bRunning;          // Is currently moving
        bool bError;            // Fault status
    };
    #pragma pack(pop)
}