#include "CellFlowOrchestrator.hpp"

#include "Logger/Logger.hpp"

#include <random>

namespace core::sim
{
    CellFlowOrchestrator::CellFlowOrchestrator(std::shared_ptr<ConveyorSimulator> entryConveyor,
                                               std::shared_ptr<ConveyorSimulator> exitConveyor,
                                               std::shared_ptr<RobotSimulator> robot,
                                               std::shared_ptr<LaserSimulator> laser,
                                               Config config)
      : m_entryConveyor(std::move(entryConveyor))
      , m_exitConveyor(std::move(exitConveyor))
      , m_robot(std::move(robot))
      , m_laser(std::move(laser))
      , m_config(config)
    {
    }

    auto CellFlowOrchestrator::start() -> void
    {
        if (!m_config.enabled) {
            return;
        }

        m_running = true;
        m_stage = Stage::WaitingEntryPart;
        m_currentPart.reset();
        m_cameraPart.reset();
        m_laserPart.reset();
        m_rejectBinParts.clear();
        m_activeJobId = 0;
        m_jobInProgress = false;
        m_jobObservedMotion = false;
        m_stageTimer = 0.0;

        if (m_robot) {
            m_robot->setGripper(false);
        }

        if (m_entryConveyor) {
            m_entryConveyor->setAutoLogic(m_config.autoLogicConveyors);
            m_entryConveyor->setAutoSpawn(m_config.autoSpawnEntry);
            m_entryConveyor->setRunning(true);
        }

        if (m_exitConveyor) {
            m_exitConveyor->setAutoLogic(m_config.autoLogicConveyors);
            m_exitConveyor->setAutoSpawn(false);
            m_exitConveyor->setRunning(true);
        }

        if (m_laser) {
            m_laser->acknowledgeDone();
        }

        logger::info("CellFlowOrchestrator started");
    }

    auto CellFlowOrchestrator::stop() -> void
    {
        m_running = false;
        m_stage = Stage::Idle;
        m_currentPart.reset();
        m_cameraPart.reset();
        m_laserPart.reset();
        m_rejectBinParts.clear();
        m_activeJobId = 0;
        m_jobInProgress = false;
        m_jobObservedMotion = false;
        m_stageTimer = 0.0;
        if (m_robot) {
            m_robot->setGripper(false);
        }
        logger::info("CellFlowOrchestrator stopped");
    }

    auto CellFlowOrchestrator::update(double deltaTimeSeconds) -> void
    {
        if (!m_running || !m_robot || !m_entryConveyor || !m_exitConveyor || !m_laser) {
            return;
        }

        m_stageTimer += deltaTimeSeconds;

        switch (m_stage) {
            case Stage::Idle:
                break;

            case Stage::WaitingEntryPart: {
                auto part = m_entryConveyor->peekPartAtEnd();
                if (part.has_value()) {
                    m_currentPart = std::move(part);
                    issueJob(2, Stage::PickEntry);
                }
                break;
            }

            case Stage::PickEntry:
                if (updateJobProgress(Stage::PlaceCamera)) {
                    if (auto pickedPart = m_entryConveyor->takePartAtEnd(); pickedPart.has_value()) {
                        m_currentPart = std::move(pickedPart);
                    }
                    m_robot->setGripper(true);
                    issueJob(3, Stage::PlaceCamera);
                }
                break;

            case Stage::PlaceCamera:
                if (updateJobProgress(Stage::Inspecting)) {
                    if (m_currentPart.has_value()) {
                        m_cameraPart = m_currentPart;
                    }
                    m_robot->setGripper(false);
                    m_stage = Stage::Inspecting;
                    m_stageTimer = 0.0;
                }
                break;

            case Stage::Inspecting:
                if (m_stageTimer >= m_config.inspectionDurationSeconds) {
                    static std::mt19937 rng{ std::random_device{}() };
                    static std::uniform_real_distribution<double> dist(0.0, 1.0);
                    const bool accepted = dist(rng) >= m_config.inspectionRejectRate;

                    if (m_cameraPart.has_value()) {
                        m_cameraPart->type = accepted ? static_cast<uint8_t>(2) : static_cast<uint8_t>(1);
                    }

                    if (accepted) {
                        issueJob(4, Stage::PickCamera);
                    }
                    else {
                        if (m_cameraPart.has_value()) {
                            m_currentPart = m_cameraPart;
                            m_cameraPart.reset();
                        }
                        m_robot->setGripper(true);
                        issueJob(1, Stage::RejectPart);
                    }
                }
                break;

            case Stage::PickCamera:
                if (updateJobProgress(Stage::PlaceLaser)) {
                    if (m_cameraPart.has_value()) {
                        m_currentPart = m_cameraPart;
                        m_cameraPart.reset();
                    }
                    m_robot->setGripper(true);
                    issueJob(5, Stage::PlaceLaser);
                }
                break;

            case Stage::PlaceLaser:
                if (updateJobProgress(Stage::Marking)) {
                    if (m_currentPart.has_value()) {
                        m_laserPart = m_currentPart;
                    }
                    m_robot->setGripper(false);
                    if (m_laser->startLocalMarking(m_config.laserMarkDurationSeconds)) {
                        m_stage = Stage::Marking;
                        m_stageTimer = 0.0;
                    }
                }
                break;

            case Stage::Marking:
                if (m_laser->state() == LaserSimulator::State::Done) {
                    m_laser->acknowledgeDone();
                    issueJob(6, Stage::PickLaser);
                }
                break;

            case Stage::PickLaser:
                if (updateJobProgress(Stage::PlaceExit)) {
                    if (m_laserPart.has_value()) {
                        m_currentPart = m_laserPart;
                        m_laserPart.reset();
                    }
                    m_robot->setGripper(true);
                    issueJob(7, Stage::PlaceExit);
                }
                break;

            case Stage::PlaceExit:
                if (updateJobProgress(Stage::ReturnHome)) {
                    if (m_currentPart.has_value()) {
                        m_exitConveyor->spawnPart(m_currentPart->type);
                    }
                    m_robot->setGripper(false);
                    issueJob(1, Stage::ReturnHome);
                }
                break;

            case Stage::ReturnHome:
                if (updateJobProgress(Stage::WaitingEntryPart)) {
                    finishCycle(true);
                }
                break;

            case Stage::RejectPart:
                if (updateJobProgress(Stage::WaitingEntryPart)) {
                    if (m_currentPart.has_value()) {
                        if (m_rejectBinParts.size() >= 24) {
                            m_rejectBinParts.erase(m_rejectBinParts.begin());
                        }
                        m_rejectBinParts.push_back(*m_currentPart);
                    }
                    m_robot->setGripper(false);
                    finishCycle(false);
                }
                break;
        }
    }

    auto CellFlowOrchestrator::isRunning() const -> bool { return m_running; }

    auto CellFlowOrchestrator::hasRobotPart() const -> bool
    {
        return m_currentPart.has_value() && m_robot && m_robot->isGripperGripped();
    }

    auto CellFlowOrchestrator::robotPartType() const -> uint8_t
    {
        return hasRobotPart() ? m_currentPart->type : 0;
    }

    auto CellFlowOrchestrator::hasCameraPart() const -> bool { return m_cameraPart.has_value(); }

    auto CellFlowOrchestrator::cameraPartType() const -> uint8_t
    {
        return m_cameraPart.has_value() ? m_cameraPart->type : 0;
    }

    auto CellFlowOrchestrator::hasLaserPart() const -> bool { return m_laserPart.has_value(); }

    auto CellFlowOrchestrator::laserPartType() const -> uint8_t
    {
        return m_laserPart.has_value() ? m_laserPart->type : 0;
    }

    auto CellFlowOrchestrator::rejectBinCount() const -> int
    {
        return static_cast<int>(m_rejectBinParts.size());
    }

    auto CellFlowOrchestrator::statusText() const -> std::string
    {
        switch (m_stage) {
            case Stage::Idle:
                return "Idle";
            case Stage::WaitingEntryPart:
                return "Waiting entry part";
            case Stage::PickEntry:
                return "Robot picking entry";
            case Stage::PlaceCamera:
                return "Robot placing camera";
            case Stage::Inspecting:
                return "Camera inspection";
            case Stage::PickCamera:
                return "Robot picking camera";
            case Stage::PlaceLaser:
                return "Robot placing laser";
            case Stage::Marking:
                return "Laser marking";
            case Stage::PickLaser:
                return "Robot picking laser";
            case Stage::PlaceExit:
                return "Robot placing exit";
            case Stage::ReturnHome:
                return "Robot return home";
            case Stage::RejectPart:
                return "Rejecting part";
            default:
                return "Unknown";
        }
    }

    auto CellFlowOrchestrator::issueJob(uint16_t jobId, Stage nextStage) -> void
    {
        m_robot->triggerJob(jobId);
        m_activeJobId = jobId;
        m_jobInProgress = true;
        m_jobObservedMotion = false;
        m_stage = nextStage;
        m_stageTimer = 0.0;
    }

    auto CellFlowOrchestrator::updateJobProgress(Stage onCompletedStage) -> bool
    {
        if (!m_jobInProgress) {
            return false;
        }

        const auto status = m_robot->status();
        if (status.bInMotion) {
            m_jobObservedMotion = true;
            return false;
        }

        if ((!m_jobObservedMotion && m_stageTimer < 0.25) || status.nJobIdFeedback != m_activeJobId) {
            return false;
        }

        m_jobInProgress = false;
        m_jobObservedMotion = false;
        m_stage = onCompletedStage;
        m_stageTimer = 0.0;
        return true;
    }

    auto CellFlowOrchestrator::finishCycle(bool accepted) -> void
    {
        m_currentPart.reset();
        m_cameraPart.reset();
        m_laserPart.reset();
        m_stage = Stage::WaitingEntryPart;
        m_stageTimer = 0.0;
        logger::info("CellFlowOrchestrator cycle completed ({})", accepted ? "accepted" : "rejected");
    }
}
