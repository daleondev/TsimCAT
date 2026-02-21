#pragma once

#include "ConveyorSimulator.hpp"
#include "LaserSimulator.hpp"
#include "Part.hpp"
#include "RobotSimulator.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace core::sim
{
    class CellFlowOrchestrator
    {
      public:
        struct Config
        {
            bool enabled{ true };
            bool cameraInternal{ true };
            double inspectionRejectRate{ 0.2 };
            bool autoSpawnEntry{ true };
            bool autoLogicConveyors{ true };
            double inspectionDurationSeconds{ 0.8 };
            double laserMarkDurationSeconds{ 2.0 };
        };

        CellFlowOrchestrator(std::shared_ptr<ConveyorSimulator> entryConveyor,
                             std::shared_ptr<ConveyorSimulator> exitConveyor,
                             std::shared_ptr<RobotSimulator> robot,
                             std::shared_ptr<LaserSimulator> laser,
                             Config config);

        auto start() -> void;
        auto stop() -> void;
        auto update(double deltaTimeSeconds) -> void;
        auto setCameraInternalMode(bool internalMode) -> void;

        auto isRunning() const -> bool;
        auto statusText() const -> std::string;
        auto hasRobotPart() const -> bool;
        auto robotPartType() const -> uint8_t;
        auto hasCameraPart() const -> bool;
        auto cameraPartType() const -> uint8_t;
        auto hasLaserPart() const -> bool;
        auto laserPartType() const -> uint8_t;
        auto rejectBinCount() const -> int;

      private:
        enum class Stage
        {
            Idle,
            WaitingEntryPart,
            PickEntry,
            PlaceCamera,
            Inspecting,
            PickCamera,
            PlaceLaser,
            Marking,
            PickLaser,
            PlaceExit,
            ReturnHome,
            RejectPart
        };

        static auto stageName(Stage stage) -> std::string_view;
        auto issueJob(uint16_t jobId, Stage nextStage) -> void;
        auto updateJobProgress(Stage onCompletedStage) -> bool;
        auto finishCycle(bool accepted) -> void;

        std::shared_ptr<ConveyorSimulator> m_entryConveyor;
        std::shared_ptr<ConveyorSimulator> m_exitConveyor;
        std::shared_ptr<RobotSimulator> m_robot;
        std::shared_ptr<LaserSimulator> m_laser;
        Config m_config;

        bool m_running{ false };
        Stage m_stage{ Stage::Idle };
        std::optional<Part> m_currentPart;
        std::optional<Part> m_cameraPart;
        std::optional<Part> m_laserPart;
        std::vector<Part> m_rejectBinParts;

        uint16_t m_activeJobId{ 0 };
        bool m_jobInProgress{ false };
        bool m_jobObservedMotion{ false };
        double m_stageTimer{ 0.0 };
        uint64_t m_cycleId{ 0 };
        uint64_t m_eventSeq{ 0 };
        Stage m_lastTracedStage{ Stage::Idle };
    };
}
