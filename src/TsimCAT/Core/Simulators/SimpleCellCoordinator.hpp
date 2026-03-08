#pragma once

#include "ConveyorSimulator.hpp"
#include "RobotSimulator.hpp"
#include "RotaryTableSimulator.hpp"

#include "Link/ILink.hpp"

#include <cstdint>
#include <memory>

namespace core::sim
{
    class SimpleCellCoordinator
    {
      public:
        struct Config
        {
            bool enabled{ true };
            bool cyclePartTypes{ true };
            int markingDelayMs{ 900 };
            int idleLoadDelayMs{ 500 };
        };

        SimpleCellCoordinator(Config config,
                              std::shared_ptr<link::ILink> link,
                              std::shared_ptr<RotaryTableSimulator> rotaryTable,
                              std::shared_ptr<RobotSimulator> robot,
                              std::shared_ptr<ConveyorSimulator> exitConveyor,
                              RobotSimulator::AdsSymbols robotSymbols,
                              RotaryTableSimulator::AdsSymbols rotarySymbols,
                              std::string exitRunSymbol);

        auto update(double deltaTimeSeconds) -> void;
        auto laserStationHasPart() const -> bool { return m_laserStationHasPart; }
        auto laserStationPartType() const -> uint8_t { return m_laserStationPartType; }

      private:
        enum class FlowState
        {
            WaitTableReady,
            WaitPickEntryDone,
            WaitPlaceLaserDone,
            WaitMarkingDone,
            WaitPickLaserDone,
            WaitPlaceExitDone,
            WaitHomeDone
        };

        auto dispatchRobotJob(uint16_t jobId, uint8_t partType) -> void;
        auto requestRotaryLoad(uint8_t partType) -> void;
        auto requestRotaryIndex(uint8_t partType) -> void;
        auto ensureExitConveyorRunning() -> void;
        auto nextPartType() -> uint8_t;

        Config m_config;
        std::shared_ptr<link::ILink> m_link;
        std::shared_ptr<RotaryTableSimulator> m_rotaryTable;
        std::shared_ptr<RobotSimulator> m_robot;
        std::shared_ptr<ConveyorSimulator> m_exitConveyor;
        RobotSimulator::AdsSymbols m_robotSymbols;
        RotaryTableSimulator::AdsSymbols m_rotarySymbols;
        std::string m_exitRunSymbol;
        FlowState m_state{ FlowState::WaitTableReady };
        double m_markingTimerSeconds{ 0.0 };
        double m_idleLoadTimerSeconds{ 0.0 };
        bool m_laserStationHasPart{ false };
        uint8_t m_laserStationPartType{ 0 };
        uint8_t m_activePartType{ 1 };
        uint8_t m_lastSpawnedPartType{ 0 };
    };
}