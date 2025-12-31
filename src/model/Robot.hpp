#pragma once

#include <cstdint>

namespace tsim::model
{
    /**
     * Commands sent from the PLC to the Robot.
     * Uses bitfields to map to PLC BIT types (packed).
     */
    #pragma pack(push, 1)
    struct RobotControl
    {
        uint16_t nJobId;            // Job number to execute
        uint8_t nPartType;          // Type of part to process
        
        // Control Bits (1 byte total)
        uint8_t bMoveEnable : 1;    // Bit 0: Permission to move
        uint8_t bReset : 1;         // Bit 1: Reset robot errors
        uint8_t reserved : 6;

        uint8_t nAreaFree_PLC;      // Bitmask: PLC signals if Area [0..7] is free for robot
    };

    /**
     * Status reported from the Robot simulation back to the PLC.
     */
    struct RobotStatus
    {
        uint16_t nJobIdFeedback;    // Currently active / last completed job
        uint8_t nPartTypeMirrored;  // Echo of the received part type
        
        // Status Bits (1 byte total)
        uint8_t bInMotion : 1;      // Bit 0: Robot is currently moving
        uint8_t bInHome : 1;        // Bit 1: Robot is in safe home position
        uint8_t bEnabled : 1;       // Bit 2: Robot power is on
        uint8_t bError : 1;         // Bit 3: Robot is in fault state
        uint8_t bBrakeTestOk : 1;   // Bit 4: Brake test successful
        uint8_t bMasteringOk : 1;   // Bit 5: Mastering successful
        uint8_t reserved1 : 2;

        // Mode Bits (1 byte total)
        uint8_t bInT1 : 1;          // Bit 0: Manual mode (Reduced speed)
        uint8_t bInT2 : 1;          // Bit 1: Manual mode (Full speed)
        uint8_t bInAut : 1;         // Bit 2: Automatic mode
        uint8_t bInExt : 1;         // Bit 3: External Automatic mode
        uint8_t reserved2 : 4;

        uint8_t nAreaFree_Robot;    // Bitmask: Robot signals if Area [0..7] is free for PLC
        uint32_t nErrorCode;        // Active error code
    };
    #pragma pack(pop)
}
