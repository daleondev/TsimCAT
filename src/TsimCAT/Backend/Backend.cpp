#include "Backend.h"
#include "Controllers/ConveyorController.h"
#include "Controllers/GantryController.h"
#include "Controllers/LaserController.h"
#include "Controllers/RobotController.h"
#include "Link/LinkFactory.hpp"
#include "Logger/Logger.hpp"
#include "Simulators/CellFlowOrchestrator.hpp"
#include "Simulators/ConveyorSimulator.hpp"
#include "Simulators/GantrySimulator.hpp"
#include "Simulators/LaserSimulator.hpp"
#include "Simulators/RobotSimulator.hpp"
#include <QCoroTimer>
#include <QDateTime>
#include <QDir>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <chrono>

using namespace std::chrono_literals;

namespace backend
{
    Backend::Backend(QObject* parent)
      : QObject(parent)
    {
        const auto configPath = qEnvironmentVariable("TSIMCAT_CONFIG", QStringLiteral("config/runtime.json"));
        QString configDiagnostics;
        m_runtimeConfig = RuntimeConfig::loadFromFile(configPath, &configDiagnostics);

        // Initialize logger
        core::logger::Logger::instance().init(m_runtimeConfig.loggerFilePath);
        core::logger::info("Backend initialized");
        core::logger::info("{}", configDiagnostics.toStdString());

        // 1. Create Shared Links
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

        // 2. Create Simulators
        m_laserSim = std::make_shared<core::sim::LaserSimulator>(m_tcpLink);
        m_robotSim = std::make_shared<core::sim::RobotSimulator>(m_adsLink);
        m_laserSim->setInternalMode(m_runtimeConfig.simulation.laser.internal);
        m_robotSim->setInternalMode(m_runtimeConfig.simulation.robot.internal);

        m_entryConveyorSim =
          std::make_shared<core::sim::ConveyorSimulator>(m_runtimeConfig.entryConveyor, m_adsLink);
        m_exitConveyorSim =
          std::make_shared<core::sim::ConveyorSimulator>(m_runtimeConfig.exitConveyor, m_adsLink);
        m_transferConveyorSim =
          std::make_shared<core::sim::ConveyorSimulator>(m_runtimeConfig.transferConveyor, m_adsLink);
        core::sim::GantrySimulator::Config gantryConfig{};
        m_gantrySim = std::make_shared<core::sim::GantrySimulator>(
          m_exitConveyorSim, m_transferConveyorSim, gantryConfig);

        m_entryConveyorSim->setInternalMode(m_runtimeConfig.simulation.entryConveyor.internal);
        m_exitConveyorSim->setInternalMode(m_runtimeConfig.simulation.exitConveyor.internal);
        m_transferConveyorSim->setInternalMode(true);
        m_entryConveyorSim->setAutoLogic(m_runtimeConfig.simulation.entryConveyor.internal);
        m_exitConveyorSim->setAutoLogic(m_runtimeConfig.simulation.exitConveyor.internal);
        m_transferConveyorSim->setAutoLogic(true);
        m_entryConveyorSim->start();
        m_exitConveyorSim->start();
        m_transferConveyorSim->start();
        m_laserSim->start();
        m_robotSim->start();
        m_gantrySim->start();
        m_gantrySim->setAutoTransfer(m_runtimeConfig.simulation.gantry.internal);

        ensureEntryConveyorCommTask();
        ensureExitConveyorCommTask();
        ensureRobotCommTask();
        ensureLaserCommTask();

        // 3. Inject into Controllers
        m_laserController = std::make_unique<backend::controllers::LaserController>(m_laserSim, this);
        m_robotController = std::make_unique<backend::controllers::RobotController>(m_robotSim, this);
        m_entryConveyorController =
          std::make_unique<backend::controllers::ConveyorController>(m_entryConveyorSim, this);
        m_exitConveyorController =
          std::make_unique<backend::controllers::ConveyorController>(m_exitConveyorSim, this);
        m_transferConveyorController =
          std::make_unique<backend::controllers::ConveyorController>(m_transferConveyorSim, this);
        m_gantryController = std::make_unique<backend::controllers::GantryController>(m_gantrySim, this);

        core::sim::CellFlowOrchestrator::Config flowConfig{};
        flowConfig.enabled = m_runtimeConfig.simulation.cellFlow.enabled;
        flowConfig.inspectionRejectRate = m_runtimeConfig.simulation.cellFlow.inspectionRejectRate;
        flowConfig.autoLogicConveyors = true;
        flowConfig.autoSpawnEntry = true;
        m_cellFlow = std::make_unique<core::sim::CellFlowOrchestrator>(
          m_entryConveyorSim, m_exitConveyorSim, m_robotSim, m_laserSim, flowConfig);

        if (m_runtimeConfig.simulation.cellFlow.enabled && m_runtimeConfig.simulation.cellFlow.autoStart &&
            m_runtimeConfig.simulation.robot.internal && m_runtimeConfig.simulation.laser.internal &&
            m_runtimeConfig.simulation.gantry.internal && m_runtimeConfig.simulation.entryConveyor.internal &&
            m_runtimeConfig.simulation.exitConveyor.internal) {
            startInternalCellFlow();
        }
        else {
            updateCellFlowStatusText();
        }

        // 4. Simulation Loop (10ms ~ 100Hz)
        [](Backend* self) -> QCoro::Task<void> {
            auto lastTime = std::chrono::steady_clock::now();
            while (true) {
                auto now = std::chrono::steady_clock::now();
                double dt = std::chrono::duration<double>(now - lastTime).count();
                lastTime = now;

                if (self->m_laserSim)
                    self->m_laserSim->update(dt);
                if (self->m_robotSim)
                    self->m_robotSim->update(dt);
                if (self->m_entryConveyorSim)
                    self->m_entryConveyorSim->update(dt);
                if (self->m_exitConveyorSim)
                    self->m_exitConveyorSim->update(dt);
                if (self->m_transferConveyorSim)
                    self->m_transferConveyorSim->update(dt);
                if (self->m_gantrySim)
                    self->m_gantrySim->update(dt);
                if (self->m_internalCellFlowRunning && self->m_cellFlow) {
                    self->m_cellFlow->update(dt);
                    self->updateCellFlowStatusText();
                }

                // Notify UI about state changes
                if (self->m_entryConveyorController)
                    emit self->m_entryConveyorController->stateChanged();
                if (self->m_exitConveyorController)
                    emit self->m_exitConveyorController->stateChanged();
                if (self->m_transferConveyorController)
                    emit self->m_transferConveyorController->stateChanged();
                if (self->m_gantryController)
                    emit self->m_gantryController->stateChanged();
                emit self->partVisualizationChanged();

                co_await QCoro::sleepFor(10ms);
            }
        }(this);
    }

    Backend::~Backend() = default;

    QString Backend::welcomeMessage() const { return QStringLiteral("Hello from C++ Backend!"); }

    QString Backend::asyncTestStatus() const { return m_asyncTestStatus; }

    backend::controllers::LaserController* Backend::laser() const { return m_laserController.get(); }

    backend::controllers::RobotController* Backend::robot() const { return m_robotController.get(); }

    backend::controllers::GantryController* Backend::gantry() const { return m_gantryController.get(); }

    backend::controllers::ConveyorController* Backend::entryConveyor() const
    {
        return m_entryConveyorController.get();
    }

    backend::controllers::ConveyorController* Backend::exitConveyor() const
    {
        return m_exitConveyorController.get();
    }

    backend::controllers::ConveyorController* Backend::transferConveyor() const
    {
        return m_transferConveyorController.get();
    }

    bool Backend::internalCellFlowRunning() const { return m_internalCellFlowRunning; }

    QString Backend::internalCellFlowStatus() const { return m_internalCellFlowStatus; }

    bool Backend::localRobotMode() const { return m_runtimeConfig.simulation.robot.internal; }

    bool Backend::localLaserMode() const { return m_runtimeConfig.simulation.laser.internal; }

    bool Backend::localGantryMode() const { return m_runtimeConfig.simulation.gantry.internal; }

    bool Backend::localEntryConveyorMode() const { return m_runtimeConfig.simulation.entryConveyor.internal; }

    bool Backend::localExitConveyorMode() const { return m_runtimeConfig.simulation.exitConveyor.internal; }

    bool Backend::robotCarriedPartVisible() const { return m_cellFlow ? m_cellFlow->hasRobotPart() : false; }

    int Backend::robotCarriedPartType() const
    {
        return m_cellFlow ? static_cast<int>(m_cellFlow->robotPartType()) : 0;
    }

    bool Backend::cameraPartVisible() const { return m_cellFlow ? m_cellFlow->hasCameraPart() : false; }

    int Backend::cameraPartType() const
    {
        return m_cellFlow ? static_cast<int>(m_cellFlow->cameraPartType()) : 0;
    }

    bool Backend::laserPartVisible() const { return m_cellFlow ? m_cellFlow->hasLaserPart() : false; }

    int Backend::laserPartType() const
    {
        return m_cellFlow ? static_cast<int>(m_cellFlow->laserPartType()) : 0;
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
        if (!enabled && m_internalCellFlowRunning) {
            stopInternalCellFlow();
        }
        emit stationModesChanged();
        updateCellFlowStatusText();
    }

    void Backend::setLocalLaserMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.laser.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.laser.internal = enabled;
        if (m_laserSim) {
            m_laserSim->setInternalMode(enabled);
        }
        ensureLaserCommTask();
        if (!enabled && m_internalCellFlowRunning) {
            stopInternalCellFlow();
        }
        emit stationModesChanged();
        updateCellFlowStatusText();
    }

    void Backend::setLocalGantryMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.gantry.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.gantry.internal = enabled;
        if (m_gantrySim) {
            m_gantrySim->setAutoTransfer(enabled);
        }
        if (!enabled && m_internalCellFlowRunning) {
            stopInternalCellFlow();
        }
        emit stationModesChanged();
        updateCellFlowStatusText();
    }

    void Backend::setLocalEntryConveyorMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.entryConveyor.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.entryConveyor.internal = enabled;
        if (m_entryConveyorSim) {
            m_entryConveyorSim->setInternalMode(enabled);
            m_entryConveyorSim->setAutoLogic(enabled);
        }
        ensureEntryConveyorCommTask();
        if (!enabled && m_internalCellFlowRunning) {
            stopInternalCellFlow();
        }
        emit stationModesChanged();
        updateCellFlowStatusText();
    }

    void Backend::setLocalExitConveyorMode(bool enabled)
    {
        if (m_runtimeConfig.simulation.exitConveyor.internal == enabled) {
            return;
        }

        m_runtimeConfig.simulation.exitConveyor.internal = enabled;
        if (m_exitConveyorSim) {
            m_exitConveyorSim->setInternalMode(enabled);
            m_exitConveyorSim->setAutoLogic(enabled);
        }
        ensureExitConveyorCommTask();
        if (!enabled && m_internalCellFlowRunning) {
            stopInternalCellFlow();
        }
        emit stationModesChanged();
        updateCellFlowStatusText();
    }

    void Backend::runAsyncTest() { doAsyncTest(); }

    void Backend::captureScreenshot(QObject* item, const QString& filename)
    {
        auto* quickItem = qobject_cast<QQuickItem*>(item);
        if (!quickItem) {
            core::logger::error("captureScreenshot: Item is null or not a QQuickItem");
            return;
        }

        auto result = quickItem->grabToImage();
        if (result) {
            connect(result.data(), &QQuickItemGrabResult::ready, this, [this, result, filename]() {
                QDir dir("screenshots");
                if (!m_runtimeConfig.screenshotDirectory.isEmpty()) {
                    dir = QDir(m_runtimeConfig.screenshotDirectory);
                }
                if (!dir.exists())
                    dir.mkpath(".");

                QString finalFilename;
                if (filename.isEmpty()) {
                    finalFilename =
                      dir.filePath(QString("capture_%1.png")
                                     .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")));
                }
                else {
                    finalFilename = dir.filePath(QString("%1.png").arg(filename));
                }

                if (result->saveToFile(finalFilename)) {
                    core::logger::info("Screenshot saved to {}", finalFilename.toStdString());
                }
                else {
                    core::logger::error("Failed to save screenshot to {}", finalFilename.toStdString());
                }
            });
        }
    }

    void Backend::startInternalCellFlow()
    {
        if (!m_cellFlow || m_internalCellFlowRunning) {
            return;
        }
        if (!m_runtimeConfig.simulation.robot.internal || !m_runtimeConfig.simulation.laser.internal ||
            !m_runtimeConfig.simulation.gantry.internal ||
            !m_runtimeConfig.simulation.entryConveyor.internal ||
            !m_runtimeConfig.simulation.exitConveyor.internal) {
            core::logger::warn("Cannot start internal cell flow: one or more stations are not in local mode");
            return;
        }

        m_cellFlow->start();
        m_internalCellFlowRunning = true;
        m_internalCellFlowStatus = QString::fromStdString(m_cellFlow->statusText());
        emit internalCellFlowRunningChanged();
        emit internalCellFlowStatusChanged();
    }

    void Backend::stopInternalCellFlow()
    {
        if (!m_cellFlow || !m_internalCellFlowRunning) {
            return;
        }

        m_cellFlow->stop();
        m_internalCellFlowRunning = false;
        m_internalCellFlowStatus = QString::fromStdString(m_cellFlow->statusText());
        emit internalCellFlowRunningChanged();
        emit internalCellFlowStatusChanged();
    }

    bool Backend::setStationLocalMode(const QString& station, bool enabled)
    {
        const auto normalized = station.trimmed().toLower();
        if (normalized == "robot") {
            setLocalRobotMode(enabled);
            return true;
        }
        if (normalized == "laser") {
            setLocalLaserMode(enabled);
            return true;
        }
        if (normalized == "entryconveyor" || normalized == "entry_conveyor" || normalized == "entry") {
            setLocalEntryConveyorMode(enabled);
            return true;
        }
        if (normalized == "exitconveyor" || normalized == "exit_conveyor" || normalized == "exit") {
            setLocalExitConveyorMode(enabled);
            return true;
        }
        if (normalized == "gantry") {
            setLocalGantryMode(enabled);
            return true;
        }
        return false;
    }

    void Backend::ensureRobotCommTask()
    {
        if (m_robotCommTaskStarted || !m_robotSim || m_runtimeConfig.simulation.robot.internal) {
            return;
        }

        m_robotCommTaskStarted = true;
        [](std::shared_ptr<core::sim::RobotSimulator> sim) -> core::coro::Task<void> {
            auto res = co_await sim->initialize();
            if (res) {
                co_await sim->run();
            }
        }(m_robotSim);
    }

    void Backend::ensureLaserCommTask()
    {
        if (m_laserCommTaskStarted || !m_laserSim || m_runtimeConfig.simulation.laser.internal) {
            return;
        }

        m_laserCommTaskStarted = true;
        [](std::shared_ptr<core::sim::LaserSimulator> sim) -> core::coro::Task<void> {
            auto res = co_await sim->initialize();
            if (res) {
                co_await sim->run();
            }
        }(m_laserSim);
    }

    void Backend::ensureEntryConveyorCommTask()
    {
        if (m_entryConveyorCommTaskStarted || !m_entryConveyorSim ||
            m_runtimeConfig.simulation.entryConveyor.internal) {
            return;
        }

        m_entryConveyorCommTaskStarted = true;
        [](std::shared_ptr<core::sim::ConveyorSimulator> sim) -> core::coro::Task<void> {
            co_await sim->run();
        }(m_entryConveyorSim);
    }

    void Backend::ensureExitConveyorCommTask()
    {
        if (m_exitConveyorCommTaskStarted || !m_exitConveyorSim ||
            m_runtimeConfig.simulation.exitConveyor.internal) {
            return;
        }

        m_exitConveyorCommTaskStarted = true;
        [](std::shared_ptr<core::sim::ConveyorSimulator> sim) -> core::coro::Task<void> {
            co_await sim->run();
        }(m_exitConveyorSim);
    }

    void Backend::updateCellFlowStatusText()
    {
        QString status;
        if (m_internalCellFlowRunning && m_cellFlow) {
            status = QString::fromStdString(m_cellFlow->statusText());
        }
        else {
            const bool allLocal = m_runtimeConfig.simulation.robot.internal &&
                                  m_runtimeConfig.simulation.laser.internal &&
                                  m_runtimeConfig.simulation.gantry.internal &&
                                  m_runtimeConfig.simulation.entryConveyor.internal &&
                                  m_runtimeConfig.simulation.exitConveyor.internal;
            status = allLocal ? QStringLiteral("Idle")
                              : QStringLiteral("Disabled (requires local mode for all stations)");
        }

        if (status != m_internalCellFlowStatus) {
            m_internalCellFlowStatus = status;
            emit internalCellFlowStatusChanged();
        }
    }

    QCoro::Task<void> Backend::doAsyncTest()
    {
        m_asyncTestStatus = "Testing QCoro... (Waiting 2s)";
        emit asyncTestStatusChanged();

        co_await QCoro::sleepFor(2s);

        m_asyncTestStatus = "QCoro Works! (Delay finished)";
        emit asyncTestStatusChanged();

        co_return;
    }
}
