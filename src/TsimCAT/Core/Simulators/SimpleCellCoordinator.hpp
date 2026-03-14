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
                              std::string exitRunSymbol,
                              std::string laserSensorSymbol = "MAIN.bLaserPartPresent");

        auto update(double deltaTimeSeconds) -> void;
        auto reset() -> void;
        auto laserStationHasPart() const -> bool { return m_laserStationHasPart; }
        auto setEnabled(bool enabled) -> void { m_config.enabled = enabled; }
        auto enabled() const -> bool { return m_config.enabled; }
        auto setTableSimulationEnabled(bool enabled) -> void { m_tableSimulationEnabled = enabled; }
        auto tableSimulationEnabled() const -> bool { return m_tableSimulationEnabled; }
        auto setRobotSimulationEnabled(bool enabled) -> void { m_robotSimulationEnabled = enabled; }
        auto robotSimulationEnabled() const -> bool { return m_robotSimulationEnabled; }
        auto setLaserSimulationEnabled(bool enabled) -> void { m_laserSimulationEnabled = enabled; }
        auto laserSimulationEnabled() const -> bool { return m_laserSimulationEnabled; }
        auto setConveyorSimulationEnabled(bool enabled) -> void { m_conveyorSimulationEnabled = enabled; }
        auto conveyorSimulationEnabled() const -> bool { return m_conveyorSimulationEnabled; }
        auto setAutoSpawnParts(bool enabled) -> void { m_autoSpawnParts = enabled; }
        auto autoSpawnParts() const -> bool { return m_autoSpawnParts; }

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

        auto dispatchRobotJob(uint16_t jobId) -> void;
        auto requestRotaryLoad() -> void;
        auto requestRotaryIndex() -> void;
        auto ensureExitConveyorRunning() -> void;

        Config m_config;
        std::shared_ptr<link::ILink> m_link;
        std::shared_ptr<RotaryTableSimulator> m_rotaryTable;
        std::shared_ptr<RobotSimulator> m_robot;
        std::shared_ptr<ConveyorSimulator> m_exitConveyor;
        RobotSimulator::AdsSymbols m_robotSymbols;
        RotaryTableSimulator::AdsSymbols m_rotarySymbols;
        std::string m_exitRunSymbol;
        std::string m_laserSensorSymbol;
        FlowState m_state{ FlowState::WaitTableReady };
        bool m_tableSimulationEnabled{ true };
        bool m_robotSimulationEnabled{ true };
        bool m_laserSimulationEnabled{ true };
        bool m_conveyorSimulationEnabled{ true };
        bool m_autoSpawnParts{ true };
        double m_markingTimerSeconds{ 0.0 };
        double m_idleLoadTimerSeconds{ 0.0 };
        bool m_laserStationHasPart{ false };
    };
}