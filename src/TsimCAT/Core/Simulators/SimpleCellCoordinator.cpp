#include "SimpleCellCoordinator.hpp"

#include "Link/Symbolic/LocalAdsLink.hpp"

namespace core::sim
{
    SimpleCellCoordinator::SimpleCellCoordinator(Config config,
                                                 std::shared_ptr<link::ILink> link,
                                                 std::shared_ptr<RotaryTableSimulator> rotaryTable,
                                                 std::shared_ptr<RobotSimulator> robot,
                                                 std::shared_ptr<ConveyorSimulator> exitConveyor,
                                                 RobotSimulator::AdsSymbols robotSymbols,
                                                 RotaryTableSimulator::AdsSymbols rotarySymbols,
                                                 std::string exitRunSymbol)
      : m_config(std::move(config))
      , m_link(std::move(link))
      , m_rotaryTable(std::move(rotaryTable))
      , m_robot(std::move(robot))
      , m_exitConveyor(std::move(exitConveyor))
      , m_robotSymbols(std::move(robotSymbols))
      , m_rotarySymbols(std::move(rotarySymbols))
      , m_exitRunSymbol(std::move(exitRunSymbol))
    {
    }

    auto SimpleCellCoordinator::update(double deltaTimeSeconds) -> void
    {
        if (!m_config.enabled) {
            return;
        }

        auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get());
        if (!localAds || !m_rotaryTable || !m_robot || !m_exitConveyor) {
            return;
        }

        if (m_conveyorSimulationEnabled) {
            ensureExitConveyorRunning();
        }

        const auto robotStatus = localAds->readSync<RobotStatus>(m_robotSymbols.statusSymbol);
        const auto rotaryStatus = localAds->readSync<RotaryTableStatus>(m_rotarySymbols.statusSymbol);
        const bool inMotion = robotStatus.bInMotion != 0;

        switch (m_state) {
            case FlowState::WaitTableReady:
                m_idleLoadTimerSeconds += deltaTimeSeconds;
                if (!rotaryStatus.bPartPresent) {
                    if (m_autoSpawnParts &&
                        m_idleLoadTimerSeconds * 1000.0 >= static_cast<double>(m_config.idleLoadDelayMs)) {
                        const auto partType = nextPartType();
                        if (!m_tableSimulationEnabled) {
                            (void)m_rotaryTable->tryLoadPart(partType);
                        }
                        else {
                            requestRotaryLoad(partType);
                        }
                    }
                    break;
                }

                if (!rotaryStatus.bReadyToPick) {
                    if (!m_tableSimulationEnabled) {
                        break;
                    }
                    requestRotaryIndex(rotaryStatus.nPartType > 0 ? rotaryStatus.nPartType : nextPartType());
                    break;
                }

                if (!m_robotSimulationEnabled) {
                    break;
                }

                if (!inMotion) {
                    dispatchRobotJob(2, rotaryStatus.nPartType > 0 ? rotaryStatus.nPartType : nextPartType());
                    m_state = FlowState::WaitPickEntryDone;
                }
                break;

            case FlowState::WaitPickEntryDone:
                if (!m_robotSimulationEnabled) {
                    break;
                }

                if (inMotion || robotStatus.nJobIdFeedback != 2) {
                    break;
                }

                m_activePartType = m_rotaryTable->takePartForRobot();
                if (m_activePartType == 0) {
                    m_activePartType = robotStatus.nPartTypeMirrored > 0 ? robotStatus.nPartTypeMirrored : 1;
                }

                dispatchRobotJob(3, m_activePartType);
                m_state = FlowState::WaitPlaceLaserDone;
                break;

            case FlowState::WaitPlaceLaserDone:
                if (!m_robotSimulationEnabled) {
                    break;
                }

                if (inMotion || robotStatus.nJobIdFeedback != 3) {
                    break;
                }

                m_laserStationHasPart = true;
                m_laserStationPartType = m_activePartType;
                m_markingTimerSeconds = 0.0;
                m_state = FlowState::WaitMarkingDone;
                break;

            case FlowState::WaitMarkingDone:
                if (!m_laserSimulationEnabled || !m_robotSimulationEnabled) {
                    break;
                }

                m_markingTimerSeconds += deltaTimeSeconds;
                if (m_markingTimerSeconds * 1000.0 < static_cast<double>(m_config.markingDelayMs)) {
                    break;
                }

                dispatchRobotJob(4, m_laserStationPartType);
                m_state = FlowState::WaitPickLaserDone;
                break;

            case FlowState::WaitPickLaserDone:
                if (!m_robotSimulationEnabled) {
                    break;
                }

                if (inMotion || robotStatus.nJobIdFeedback != 4) {
                    break;
                }

                m_laserStationHasPart = false;
                dispatchRobotJob(7, m_laserStationPartType > 0 ? m_laserStationPartType : m_activePartType);
                m_state = FlowState::WaitPlaceExitDone;
                break;

            case FlowState::WaitPlaceExitDone:
                if (!m_robotSimulationEnabled) {
                    break;
                }

                if (inMotion || robotStatus.nJobIdFeedback != 7) {
                    break;
                }

                m_exitConveyor->spawnPartAtPosition(m_activePartType > 0 ? m_activePartType : 1, 120.0);
                dispatchRobotJob(1, m_activePartType);
                m_state = FlowState::WaitHomeDone;
                break;

            case FlowState::WaitHomeDone:
                if (!m_robotSimulationEnabled) {
                    break;
                }

                if (inMotion || robotStatus.nJobIdFeedback != 1) {
                    break;
                }

                m_idleLoadTimerSeconds = 0.0;
                m_state = FlowState::WaitTableReady;
                break;
        }
    }

    auto SimpleCellCoordinator::dispatchRobotJob(uint16_t jobId, uint8_t partType) -> void
    {
        auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get());
        if (!localAds) {
            return;
        }

        auto control = localAds->readSync<RobotControl>(m_robotSymbols.controlSymbol);
        control.nJobId = jobId;
        control.nPartType = partType;
        control.bMoveEnable = 1;
        control.bReset = 0;
        localAds->writeSync(m_robotSymbols.controlSymbol, control);
    }

    auto SimpleCellCoordinator::requestRotaryLoad(uint8_t partType) -> void
    {
        auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get());
        if (!localAds) {
            return;
        }

        RotaryTableControl control{};
        control.bEnable = 1;
        control.bLoadPart = 1;
        control.bIndex = 0;
        control.nRequestedPartType = partType;
        localAds->writeSync(m_rotarySymbols.controlSymbol, control);
        m_lastSpawnedPartType = partType;
    }

    auto SimpleCellCoordinator::requestRotaryIndex(uint8_t partType) -> void
    {
        auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get());
        if (!localAds) {
            return;
        }

        RotaryTableControl control{};
        control.bEnable = 1;
        control.bIndex = 1;
        control.nRequestedPartType = partType;
        localAds->writeSync(m_rotarySymbols.controlSymbol, control);
    }

    auto SimpleCellCoordinator::ensureExitConveyorRunning() -> void
    {
        auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get());
        if (!localAds || m_exitRunSymbol.empty()) {
            return;
        }

        localAds->writeSync(m_exitRunSymbol, true);
    }

    auto SimpleCellCoordinator::nextPartType() -> uint8_t
    {
        if (!m_config.cyclePartTypes) {
            return 1;
        }

        m_lastSpawnedPartType = m_lastSpawnedPartType == 1 ? 2 : 1;
        return m_lastSpawnedPartType == 0 ? 1 : m_lastSpawnedPartType;
    }
}