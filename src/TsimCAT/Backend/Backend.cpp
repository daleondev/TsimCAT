#include "Backend.h"

#include "Controllers/ConveyorController.h"
#include "Controllers/RobotController.h"
#include "Link/LinkFactory.hpp"
#include "Logger/Logger.hpp"
#include "Logger/TraceLogger.hpp"
#include "Simulators/ConveyorSimulator.hpp"
#include "Simulators/RobotSimulator.hpp"

#include <QCoreApplication>
#include <QCoroTimer>
#include <chrono>
#include <filesystem>

using namespace std::chrono_literals;

namespace backend
{
    Backend::Backend(QObject* parent)
      : QObject(parent)
    {
        const auto configPath = qEnvironmentVariable("TSIMCAT_CONFIG", QStringLiteral("config/runtime.json"));
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

        m_robotSim = std::make_shared<core::sim::RobotSimulator>(
          m_adsLink,
          core::sim::RobotSimulator::AdsSymbols{ .controlSymbol = m_runtimeConfig.adsVariables.robot.control,
                                                 .statusSymbol = m_runtimeConfig.adsVariables.robot.status });

        auto entryConveyorConfig =
          core::sim::ConveyorSimulator::Config{ .name = "EntryConveyor",
                                                .length = 1875.0,
                                                .speed = 250.0,
                                                .sensorPositions = { 437.5, 1000.0, 1775.0 },
                                                .damperSensorIndex = 0,
                                                .damperCloseSensorIndex = 1,
                                                .endSensorIndex = 2,
                                                .consumeAtEndSensor = false,
                                                .damperOpenDelaySeconds = 1.0,
                                                .adsRunCmd = m_runtimeConfig.adsVariables.conveyors.entryRun,
                                                .adsSensorSignals =
                                                  m_runtimeConfig.adsVariables.conveyors.entrySensors };

        auto exitConveyorConfig =
          core::sim::ConveyorSimulator::Config{ .name = "ExitConveyor",
                                                .length = 1250.0,
                                                .speed = 250.0,
                                                .sensorPositions = { 250.0, 700.0, 1150.0 },
                                                .damperSensorIndex = 0,
                                                .damperCloseSensorIndex = 1,
                                                .endSensorIndex = 2,
                                                .consumeAtEndSensor = false,
                                                .damperOpenDelaySeconds = 0.8,
                                                .adsRunCmd = m_runtimeConfig.adsVariables.conveyors.exitRun,
                                                .adsSensorSignals =
                                                  m_runtimeConfig.adsVariables.conveyors.exitSensors };

        m_entryConveyorSim = std::make_shared<core::sim::ConveyorSimulator>(entryConveyorConfig, m_adsLink);
        m_exitConveyorSim = std::make_shared<core::sim::ConveyorSimulator>(exitConveyorConfig, m_adsLink);

        m_robotSim->setInternalMode(m_runtimeConfig.simulation.robot.internal);
        m_entryConveyorSim->setInternalMode(m_runtimeConfig.simulation.entryConveyor.internal);
        m_exitConveyorSim->setInternalMode(m_runtimeConfig.simulation.exitConveyor.internal);

        m_entryConveyorSim->setAutoLogic(true);
        m_exitConveyorSim->setAutoLogic(true);
        m_entryConveyorSim->start();
        m_exitConveyorSim->start();
        m_robotSim->start();

        ensureEntryConveyorCommTask();
        ensureExitConveyorCommTask();
        ensureRobotCommTask();

        m_robotController = std::make_unique<backend::controllers::RobotController>(m_robotSim, this);
        m_entryConveyorController =
          std::make_unique<backend::controllers::ConveyorController>(m_entryConveyorSim, this);
        m_exitConveyorController =
          std::make_unique<backend::controllers::ConveyorController>(m_exitConveyorSim, this);

        (void)[](Backend * self)->QCoro::Task<void>
        {
            auto lastTime = std::chrono::steady_clock::now();
            while (true) {
                auto now = std::chrono::steady_clock::now();
                const double dt = std::chrono::duration<double>(now - lastTime).count();
                lastTime = now;

                if (self->m_robotSim)
                    self->m_robotSim->update(dt);
                if (self->m_entryConveyorSim)
                    self->m_entryConveyorSim->update(dt);
                if (self->m_exitConveyorSim)
                    self->m_exitConveyorSim->update(dt);

                if (self->m_entryConveyorController)
                    emit self->m_entryConveyorController->stateChanged();
                if (self->m_exitConveyorController)
                    emit self->m_exitConveyorController->stateChanged();

                co_await QCoro::sleepFor(10ms);
            }
        }
        (this);
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

    backend::controllers::ConveyorController* Backend::entryConveyor() const
    {
        return m_entryConveyorController.get();
    }

    backend::controllers::ConveyorController* Backend::exitConveyor() const
    {
        return m_exitConveyorController.get();
    }

    bool Backend::localRobotMode() const { return m_runtimeConfig.simulation.robot.internal; }

    bool Backend::localEntryConveyorMode() const { return m_runtimeConfig.simulation.entryConveyor.internal; }

    bool Backend::localExitConveyorMode() const { return m_runtimeConfig.simulation.exitConveyor.internal; }

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

    void Backend::setLocalEntryConveyorMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.entryConveyor.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.entryConveyor.internal = enabled;
        if (m_entryConveyorSim) {
            m_entryConveyorSim->setInternalMode(enabled);
        }
        ensureEntryConveyorCommTask();
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

        (void)[](Backend * self)->QCoro::Task<void> { co_await self->doAsyncTest(); }
        (this);
    }

    QCoro::Task<void> Backend::doAsyncTest()
    {
        co_await QCoro::sleepFor(2s);
        m_asyncTestStatus = "Async task completed!";
        emit asyncTestStatusChanged();
    }

    void Backend::ensureRobotCommTask()
    {
        if (m_robotCommTaskStarted || !m_robotSim || m_runtimeConfig.simulation.robot.internal) {
            return;
        }

        m_robotCommTaskStarted = true;
        (void)[](std::shared_ptr<core::sim::RobotSimulator> sim)->core::coro::Task<void>
        {
            auto res = co_await sim->initialize();
            if (res) {
                co_await sim->run();
            }
        }
        (m_robotSim);
    }

    void Backend::ensureEntryConveyorCommTask()
    {
        if (m_entryConveyorCommTaskStarted || !m_entryConveyorSim ||
            m_runtimeConfig.simulation.entryConveyor.internal) {
            return;
        }

        m_entryConveyorCommTaskStarted = true;
        (void)[](std::shared_ptr<core::sim::ConveyorSimulator> sim)->core::coro::Task<void>
        {
            co_await sim->run();
        }
        (m_entryConveyorSim);
    }

    void Backend::ensureExitConveyorCommTask()
    {
        if (m_exitConveyorCommTaskStarted || !m_exitConveyorSim ||
            m_runtimeConfig.simulation.exitConveyor.internal) {
            return;
        }

        m_exitConveyorCommTaskStarted = true;
        (void)[](std::shared_ptr<core::sim::ConveyorSimulator> sim)->core::coro::Task<void>
        {
            co_await sim->run();
        }
        (m_exitConveyorSim);
    }
}
