#pragma once

#include <cstdint>
#include <array>

namespace tsim::model
{
    /**
     * Commands sent from the PLC to the Robot.
     */
    #pragma pack(push, 1)
    struct RobotControl
    {
        std::array<float, 6> fTargetJointAngles; // Target angles in degrees
        bool bEnable;                            // Power on
        bool bReset;                             // Reset errors
    };

    /**
     * Status reported from the Robot simulation back to the PLC.
     */
    struct RobotStatus
    {
        std::array<float, 6> fActualJointAngles; // Current measured angles
        bool bEnabled;                           // Power status
        bool bError;                             // Fault status
        uint32_t nErrorCode;                     // Specific error code
    };
    #pragma pack(pop)
}
