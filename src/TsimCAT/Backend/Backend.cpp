#include "Backend.h"

#include "Controllers/ConveyorController.h"
#include "Controllers/RobotController.h"
#include "Controllers/RotaryTableController.h"
#include "Link/LinkFactory.hpp"
#include "Logger/Logger.hpp"
#include "Logger/TraceLogger.hpp"
#include "ScreenshotProvider.h"
#include "Simulators/ConveyorSimulator.hpp"
#include "Simulators/RobotSimulator.hpp"
#include "Simulators/RotaryTableSimulator.hpp"
#include "Simulators/SimpleCellCoordinator.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QTimer>
#include <chrono>
#include <filesystem>
#include <numbers>

namespace backend
{
    namespace
    {
        auto makeRobotSimulatorConfig(const RuntimeConfig& runtimeConfig) -> core::sim::RobotSimulator::Config
        {
            constexpr double degToRad = std::numbers::pi / 180.0;

            core::sim::RobotSimulator::Config config;
            config.jobTrajectories.reserve(runtimeConfig.simulation.robotMotion.jobs.size());

            for (const auto& job : runtimeConfig.simulation.robotMotion.jobs) {
                core::sim::RobotSimulator::JobTrajectory trajectory;
                trajectory.jobId = job.jobId;
                trajectory.poses.reserve(job.poses.size());

                for (const auto& pose : job.poses) {
                    trajectory.poses.push_back(core::sim::Pose{ .x = pose.x,
                                                                .y = pose.y,
                                                                .z = pose.z,
                                                                .roll = pose.rollDeg * degToRad,
                                                                .pitch = pose.pitchDeg * degToRad,
                                                                .yaw = pose.yawDeg * degToRad });
                }

                if (!trajectory.poses.empty()) {
                    config.jobTrajectories.push_back(std::move(trajectory));
                }
            }

            return config;
        }
    }

    Backend::Backend(QObject* parent)
      : QObject(parent)
    {
        QString configPath = qEnvironmentVariable("TSIMCAT_CONFIG");
        if (configPath.isEmpty()) {
            const QString workspaceRelative = QStringLiteral("config/runtime.json");
            const QString appRelative =
              QCoreApplication::applicationDirPath() + QStringLiteral("/../../config/runtime.json");
            configPath = QFileInfo::exists(workspaceRelative) ? workspaceRelative : appRelative;
        }
        QString configDiagnostics;
        m_runtimeConfig = RuntimeConfig::loadFromFile(configPath, &configDiagnostics);

        if (auto loggerInit = core::logger::Logger::instance().init(m_runtimeConfig.loggerFilePath());
            !loggerInit) {
            core::logger::error("Failed to initialize logger: {}", loggerInit.error().message());
        }
        core::logger::info("Backend initialized (simple cell)");
        core::logger::info("{}", configDiagnostics.toStdString());

        {
            core::logger::TraceConfig traceConfig{};
            traceConfig.enabled = m_runtimeConfig.trace.enabled;
            traceConfig.enableProtocol = m_runtimeConfig.trace.enableProtocol;
            traceConfig.enableState = m_runtimeConfig.trace.enableState;
            traceConfig.enableFlow = m_runtimeConfig.trace.enableFlow;
            traceConfig.enableInvariant = m_runtimeConfig.trace.enableInvariant;
            traceConfig.mirrorToHumanLog = m_runtimeConfig.trace.mirrorToHumanLog;
            traceConfig.sampleIntervalMs = m_runtimeConfig.trace.sampleIntervalMs;
            traceConfig.outputFile = std::filesystem::path(
              (m_runtimeConfig.trace.outputFolder + QLatin1Char('/') + m_runtimeConfig.trace.fileName)
                .toStdString());
            traceConfig.stationFilter = m_runtimeConfig.trace.stationFilter;

            auto traceInit = core::logger::TraceLogger::instance().init(std::move(traceConfig));
            if (!traceInit) {
                core::logger::error("Failed to initialize trace logger: {}", traceInit.error().message());
            }
            else {
                core::logger::TraceLogger::instance().event(
                  core::logger::TraceCategory::Lifecycle,
                  "backend",
                  "session_started",
                  { core::logger::traceField("pid",
                                             static_cast<uint64_t>(QCoreApplication::applicationPid())),
                    core::logger::traceField("config", configPath.toStdString()) });
            }
        }

        auto tcpRes = core::link::create(core::link::Role::Server,
                                         core::link::Mode::Raw,
                                         core::link::Protocol::Tcp,
                                         m_runtimeConfig.tcpLink);
        if (tcpRes) {
            m_tcpLink = std::move(*tcpRes);
        }
        else {
            core::logger::error("Failed to create shared TCP link: {}", tcpRes.error().message());
        }

        auto adsRes = core::link::create(core::link::Role::Client,
                                         core::link::Mode::Symbolic,
                                         core::link::Protocol::Ads,
                                         m_runtimeConfig.adsLink);
        if (adsRes) {
            m_adsLink = std::move(*adsRes);
        }
        else {
            core::logger::error("Failed to create shared ADS link: {}", adsRes.error().message());
        }

        auto opcUaRes = core::link::create(core::link::Role::Client,
                                           core::link::Mode::Symbolic,
                                           core::link::Protocol::OpcUa,
                                           m_runtimeConfig.opcUaLink);
        if (!opcUaRes) {
            core::logger::info("OPC UA link is available but not active in simple cell mode: {}",
                               opcUaRes.error().message());
        }

        const auto robotSymbols =
          core::sim::RobotSimulator::AdsSymbols{ .controlSymbol = m_runtimeConfig.adsVariables.robot.control,
                                                 .statusSymbol = m_runtimeConfig.adsVariables.robot.status,
                                                 .gripperSensorSymbol =
                                                   m_runtimeConfig.adsVariables.gripper.partDetectedSensor };
        const auto rotarySymbols = core::sim::RotaryTableSimulator::AdsSymbols{
            .controlSymbol = m_runtimeConfig.adsVariables.rotaryTable.control,
            .statusSymbol = m_runtimeConfig.adsVariables.rotaryTable.status
        };
        const auto robotConfig = makeRobotSimulatorConfig(m_runtimeConfig);

        const auto exitConveyorConfig = core::sim::ConveyorSimulator::Config{
            .name = "ExitConveyor",
            .length = m_runtimeConfig.simulation.exitConveyorConfig.length,
            .speed = m_runtimeConfig.simulation.exitConveyorConfig.speed,
            .sensorPositions = m_runtimeConfig.simulation.exitConveyorConfig.sensorPositions,
            .damperSensorIndex = m_runtimeConfig.simulation.exitConveyorConfig.damperSensorIndex,
            .damperCloseSensorIndex = m_runtimeConfig.simulation.exitConveyorConfig.damperCloseSensorIndex,
            .endSensorIndex = m_runtimeConfig.simulation.exitConveyorConfig.endSensorIndex,
            .consumeAtEndSensor = m_runtimeConfig.simulation.exitConveyorConfig.consumeAtEndSensor,
            .damperOpenDelaySeconds = m_runtimeConfig.simulation.exitConveyorConfig.damperOpenDelaySeconds,
            .adsRunCmd = m_runtimeConfig.adsVariables.conveyors.exitRun,
            .adsSensorSignals = m_runtimeConfig.adsVariables.conveyors.exitSensors,
        };

        m_robotSim = std::make_shared<core::sim::RobotSimulator>(m_adsLink, robotSymbols, robotConfig);
        m_rotaryTableSim = std::make_shared<core::sim::RotaryTableSimulator>(
          core::sim::RotaryTableSimulator::Config{
            .name = "RotaryTable",
            .radius = m_runtimeConfig.simulation.rotaryTableConfig.radius,
            .height = m_runtimeConfig.simulation.rotaryTableConfig.height,
            .loadAngleDeg = m_runtimeConfig.simulation.rotaryTableConfig.loadAngleDeg,
            .pickAngleDeg = m_runtimeConfig.simulation.rotaryTableConfig.pickAngleDeg,
            .rotationSpeedDegPerSecond =
              m_runtimeConfig.simulation.rotaryTableConfig.rotationSpeedDegPerSecond,
            .loadDelaySeconds = m_runtimeConfig.simulation.rotaryTableConfig.loadDelaySeconds },
          m_adsLink,
          rotarySymbols);
        m_exitConveyorSim = std::make_shared<core::sim::ConveyorSimulator>(exitConveyorConfig, m_adsLink);

        m_robotSim->setInternalMode(m_runtimeConfig.simulation.robot.internal);
        m_rotaryTableSim->setInternalMode(m_runtimeConfig.simulation.rotaryTable.internal);
        m_exitConveyorSim->setInternalMode(m_runtimeConfig.simulation.exitConveyor.internal);

        m_exitConveyorSim->setAutoLogic(true);
        m_exitConveyorSim->setAutoSpawn(false);
        m_rotaryTableSim->start();
        m_exitConveyorSim->start();
        m_robotSim->start();

        m_cellCoordinator = std::make_shared<core::sim::SimpleCellCoordinator>(
          core::sim::SimpleCellCoordinator::Config{
            .enabled = m_runtimeConfig.simulation.localCell.enabled,
            .cyclePartTypes = m_runtimeConfig.simulation.localCell.cyclePartTypes,
            .markingDelayMs = m_runtimeConfig.simulation.localCell.markingDelayMs,
            .idleLoadDelayMs = m_runtimeConfig.simulation.localCell.idleLoadDelayMs },
          m_adsLink,
          m_rotaryTableSim,
          m_robotSim,
          m_exitConveyorSim,
          robotSymbols,
          rotarySymbols,
          m_runtimeConfig.adsVariables.conveyors.exitRun,
          m_runtimeConfig.adsVariables.laser.partPresentSensor);

        m_localSimulationEnabled = m_runtimeConfig.simulation.localCell.enabled;
        m_localTableSimulationEnabled = true;
        m_localRobotSimulationEnabled = true;
        m_localLaserSimulationEnabled = true;
        m_localConveyorSimulationEnabled = true;
        m_autoSpawnPartsEnabled = true;
        m_autoDespawnPartsEnabled = m_runtimeConfig.simulation.exitConveyorConfig.consumeAtEndSensor;

        m_cellCoordinator->setEnabled(m_localSimulationEnabled);
        m_cellCoordinator->setTableSimulationEnabled(m_localTableSimulationEnabled);
        m_cellCoordinator->setRobotSimulationEnabled(m_localRobotSimulationEnabled);
        m_cellCoordinator->setLaserSimulationEnabled(m_localLaserSimulationEnabled);
        m_cellCoordinator->setConveyorSimulationEnabled(m_localConveyorSimulationEnabled);
        m_cellCoordinator->setAutoSpawnParts(m_autoSpawnPartsEnabled);
        m_exitConveyorSim->setConsumeAtEndSensor(m_autoDespawnPartsEnabled);
        m_robotSim->setExternalCommandSimulationEnabled(m_localRobotSimulationEnabled);

        ensureRobotCommTask();
        ensureRotaryTableCommTask();
        ensureExitConveyorCommTask();

        m_robotController = std::make_unique<backend::controllers::RobotController>(m_robotSim, this);
        m_rotaryTableController =
          std::make_unique<backend::controllers::RotaryTableController>(m_rotaryTableSim, this);
        m_exitConveyorController =
          std::make_unique<backend::controllers::ConveyorController>(m_exitConveyorSim, this);

        m_screenshotProvider = std::make_unique<backend::ScreenshotProvider>(
          ScreenshotProvider::SimulatorRefs{ .robot = m_robotSim,
                                             .rotaryTable = m_rotaryTableSim,
                                             .exitConveyor = m_exitConveyorSim,
                                             .coordinator = m_cellCoordinator },
          m_runtimeConfig.analyzer.outputFolder,
          m_runtimeConfig.analyzer.frameIntervalMs,
          m_runtimeConfig.analyzer.maxFrames,
          this);

        startUpdateLoop();
    }

    Backend::~Backend()
    {
        core::logger::TraceLogger::instance().event(
          core::logger::TraceCategory::Lifecycle,
          "backend",
          "session_stopped",
          { core::logger::traceField("pid", static_cast<uint64_t>(QCoreApplication::applicationPid())) });
        core::logger::TraceLogger::instance().shutdown();
    }

    QString Backend::welcomeMessage() const { return QStringLiteral("Hello from C++ Backend!"); }

    QString Backend::asyncTestStatus() const { return m_asyncTestStatus; }

    backend::controllers::RobotController* Backend::robot() const { return m_robotController.get(); }

    backend::controllers::RotaryTableController* Backend::rotaryTable() const
    {
        return m_rotaryTableController.get();
    }

    backend::controllers::ConveyorController* Backend::exitConveyor() const
    {
        return m_exitConveyorController.get();
    }

    backend::ScreenshotProvider* Backend::screenshotProvider() const { return m_screenshotProvider.get(); }

    bool Backend::robotCarriedPartVisible() const
    {
        return m_robotSim ? m_robotSim->isGripperGripped() : false;
    }

    int Backend::robotCarriedPartType() const
    {
        return robotCarriedPartVisible() && m_robotSim
                 ? static_cast<int>(m_robotSim->status().nPartTypeMirrored)
                 : 0;
    }

    bool Backend::laserPartVisible() const
    {
        return m_cellCoordinator ? m_cellCoordinator->laserStationHasPart() : false;
    }

    int Backend::laserPartType() const
    {
        return m_cellCoordinator ? static_cast<int>(m_cellCoordinator->laserStationPartType()) : 0;
    }

    bool Backend::laserSensorBlocked() const
    {
        return m_cellCoordinator ? m_cellCoordinator->laserStationHasPart() : false;
    }

    bool Backend::usingLocalAdsShadow() const { return m_adsLink && m_runtimeConfig.adsLink.inProcess; }

    bool Backend::localPlcShadow() const { return m_runtimeConfig.simulation.localPlcShadow; }

    bool Backend::localSimulationEnabled() const { return m_localSimulationEnabled; }

    bool Backend::localTableSimulationEnabled() const { return m_localTableSimulationEnabled; }

    bool Backend::localRobotSimulationEnabled() const { return m_localRobotSimulationEnabled; }

    bool Backend::localLaserSimulationEnabled() const { return m_localLaserSimulationEnabled; }

    bool Backend::localConveyorSimulationEnabled() const { return m_localConveyorSimulationEnabled; }

    bool Backend::autoSpawnPartsEnabled() const { return m_autoSpawnPartsEnabled; }

    bool Backend::autoDespawnPartsEnabled() const { return m_autoDespawnPartsEnabled; }

    bool Backend::localRobotMode() const { return m_runtimeConfig.simulation.robot.internal; }

    bool Backend::localRotaryTableMode() const { return m_runtimeConfig.simulation.rotaryTable.internal; }

    bool Backend::localExitConveyorMode() const { return m_runtimeConfig.simulation.exitConveyor.internal; }

    void Backend::setLocalSimulationEnabled(bool enabled)
    {
        if (m_localSimulationEnabled == enabled) {
            return;
        }

        m_localSimulationEnabled = enabled;
        if (m_cellCoordinator) {
            m_cellCoordinator->setEnabled(enabled);
        }
        emit simulationSettingsChanged();
    }

    void Backend::setLocalTableSimulationEnabled(bool enabled)
    {
        if (m_localTableSimulationEnabled == enabled) {
            return;
        }

        m_localTableSimulationEnabled = enabled;
        if (m_cellCoordinator) {
            m_cellCoordinator->setTableSimulationEnabled(enabled);
        }
        emit simulationSettingsChanged();
    }

    void Backend::setLocalRobotSimulationEnabled(bool enabled)
    {
        if (m_localRobotSimulationEnabled == enabled) {
            return;
        }

        m_localRobotSimulationEnabled = enabled;
        if (m_cellCoordinator) {
            m_cellCoordinator->setRobotSimulationEnabled(enabled);
        }
        if (m_robotSim) {
            m_robotSim->setExternalCommandSimulationEnabled(enabled);
        }
        emit simulationSettingsChanged();
    }

    void Backend::setLocalLaserSimulationEnabled(bool enabled)
    {
        if (m_localLaserSimulationEnabled == enabled) {
            return;
        }

        m_localLaserSimulationEnabled = enabled;
        if (m_cellCoordinator) {
            m_cellCoordinator->setLaserSimulationEnabled(enabled);
        }
        emit simulationSettingsChanged();
    }

    void Backend::setLocalConveyorSimulationEnabled(bool enabled)
    {
        if (m_localConveyorSimulationEnabled == enabled) {
            return;
        }

        m_localConveyorSimulationEnabled = enabled;
        if (m_cellCoordinator) {
            m_cellCoordinator->setConveyorSimulationEnabled(enabled);
        }
        emit simulationSettingsChanged();
    }

    void Backend::setAutoSpawnPartsEnabled(bool enabled)
    {
        if (m_autoSpawnPartsEnabled == enabled) {
            return;
        }

        m_autoSpawnPartsEnabled = enabled;
        if (m_cellCoordinator) {
            m_cellCoordinator->setAutoSpawnParts(enabled);
        }
        emit simulationSettingsChanged();
    }

    void Backend::setAutoDespawnPartsEnabled(bool enabled)
    {
        if (m_autoDespawnPartsEnabled == enabled) {
            return;
        }

        m_autoDespawnPartsEnabled = enabled;
        if (m_exitConveyorSim) {
            m_exitConveyorSim->setConsumeAtEndSensor(enabled);
        }
        if (m_exitConveyorController) {
            emit m_exitConveyorController->stateChanged();
        }
        emit simulationSettingsChanged();
    }

    void Backend::setLocalRobotMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.robot.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.robot.internal = enabled;
        if (m_robotSim) {
            m_robotSim->setInternalMode(enabled);
        }
        ensureRobotCommTask();
        emit stationModesChanged();
    }

    void Backend::setLocalRotaryTableMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.rotaryTable.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.rotaryTable.internal = enabled;
        if (m_rotaryTableSim) {
            m_rotaryTableSim->setInternalMode(enabled);
        }
        ensureRotaryTableCommTask();
        emit stationModesChanged();
    }

    void Backend::setLocalExitConveyorMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.exitConveyor.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.exitConveyor.internal = enabled;
        if (m_exitConveyorSim) {
            m_exitConveyorSim->setInternalMode(enabled);
        }
        ensureExitConveyorCommTask();
        emit stationModesChanged();
    }

    void Backend::runAsyncTest()
    {
        m_asyncTestStatus = "Running...";
        emit asyncTestStatusChanged();

        QTimer::singleShot(2000, this, [this]() {
            m_asyncTestStatus = "Async task completed!";
            emit asyncTestStatusChanged();
        });
    }

    void Backend::spawnTablePart()
    {
        if (!m_rotaryTableSim) {
            return;
        }

        const int partType = m_runtimeConfig.simulation.localCell.cyclePartTypes ? m_nextManualPartType : 1;
        const auto desiredType = static_cast<uint8_t>(partType);
        if (!m_rotaryTableSim->tryLoadPart(desiredType)) {
            m_rotaryTableSim->queuePart(desiredType);
        }
        if (m_runtimeConfig.simulation.localCell.cyclePartTypes) {
            m_nextManualPartType = (m_nextManualPartType == 1) ? 2 : 1;
        }
        if (m_rotaryTableController) {
            emit m_rotaryTableController->stateChanged();
        }
        emit simulationSettingsChanged();
    }

    bool Backend::despawnExitPart()
    {
        if (!m_exitConveyorSim) {
            return false;
        }

        const bool removed = m_exitConveyorSim->takePartAtEnd().has_value();
        if (removed && m_exitConveyorController) {
            emit m_exitConveyorController->stateChanged();
        }
        return removed;
    }

    void Backend::resetSimulation()
    {
        if (m_cellCoordinator) {
            m_cellCoordinator->reset();
        }

        if (m_rotaryTableController) {
            emit m_rotaryTableController->stateChanged();
        }
        if (m_robotController) {
            emit m_robotController->stateChanged();
        }
        if (m_exitConveyorController) {
            emit m_exitConveyorController->stateChanged();
        }
        emit partVisualizationChanged();
        emit simulationSettingsChanged();

        core::logger::info("Backend: Simulation reset by user");
    }

    void Backend::startBackgroundTask(core::coro::Task<void>&& task)
    {
        m_backgroundTasks.push_back(std::move(task));
        auto& backgroundTask = m_backgroundTasks.back();
        const auto handle = backgroundTask.getHandle();
        if (handle) {
            handle.resume();
        }
    }

    void Backend::startUpdateLoop()
    {
        m_lastUpdateTime = std::chrono::steady_clock::now();
        m_updateTimer = new QTimer(this);
        m_updateTimer->setInterval(16);
        connect(m_updateTimer, &QTimer::timeout, this, [this]() {
            const auto now = std::chrono::steady_clock::now();
            const double dt = std::chrono::duration<double>(now - m_lastUpdateTime).count();
            m_lastUpdateTime = now;

            if (m_cellCoordinator) {
                m_cellCoordinator->update(dt);
            }
            if (m_rotaryTableSim && m_localSimulationEnabled && m_localTableSimulationEnabled) {
                m_rotaryTableSim->update(dt);
            }
            if (m_robotSim && m_localSimulationEnabled) {
                m_robotSim->update(dt);
            }
            if (m_exitConveyorSim && m_localSimulationEnabled && m_localConveyorSimulationEnabled) {
                m_exitConveyorSim->update(dt);
            }

            if (m_rotaryTableController) {
                emit m_rotaryTableController->stateChanged();
            }
            if (m_exitConveyorController) {
                emit m_exitConveyorController->stateChanged();
            }
            emit partVisualizationChanged();
        });
        m_updateTimer->start();
    }

    void Backend::ensureRobotCommTask()
    {
        if (m_robotCommTaskStarted || !m_robotSim || m_runtimeConfig.simulation.robot.internal ||
            usingLocalAdsShadow()) {
            return;
        }

        m_robotCommTaskStarted = true;
        startBackgroundTask([sim = m_robotSim]() -> core::coro::Task<void> {
            auto res = co_await sim->initialize();
            if (res) {
                co_await sim->run();
            }
        }());
    }

    void Backend::ensureRotaryTableCommTask()
    {
        if (m_rotaryTableCommTaskStarted || !m_rotaryTableSim ||
            m_runtimeConfig.simulation.rotaryTable.internal || usingLocalAdsShadow()) {
            return;
        }

        m_rotaryTableCommTaskStarted = true;
        startBackgroundTask([sim = m_rotaryTableSim]() -> core::coro::Task<void> {
            auto res = co_await sim->initialize();
            if (res) {
                co_await sim->run();
            }
        }());
    }

    void Backend::ensureExitConveyorCommTask()
    {
        if (m_exitConveyorCommTaskStarted || !m_exitConveyorSim ||
            m_runtimeConfig.simulation.exitConveyor.internal || usingLocalAdsShadow()) {
            return;
        }

        m_exitConveyorCommTaskStarted = true;
        startBackgroundTask([sim = m_exitConveyorSim]() -> core::coro::Task<void> {
            auto res = co_await sim->initialize();
            if (res) {
                co_await sim->run();
            }
        }());
    }
}
