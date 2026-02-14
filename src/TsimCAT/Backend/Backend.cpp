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
#include <QFileInfo>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QTextStream>
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
        m_analyzerOutputFolder = resolveAnalyzerOutputFolder();
        clearAnalyzerArtifactsOnStartup();

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
                self->updateAnalyzer(dt);

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

    int Backend::rejectBinCount() const { return m_cellFlow ? m_cellFlow->rejectBinCount() : 0; }

    bool Backend::analyzerEnabled() const { return m_runtimeConfig.analyzer.enabled; }

    bool Backend::analyzerCaptureRunning() const { return m_analyzerRunning; }

    int Backend::analyzerCapturedFrames() const { return m_analyzerCapturedFrames; }

    QString Backend::analyzerOutputFolder() const { return m_analyzerOutputFolder; }

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

    bool Backend::startAnalyzerCapture(QObject* item)
    {
        if (!m_runtimeConfig.analyzer.enabled) {
            core::logger::warn("Analyzer capture start requested but analyzer.enabled=false");
            return false;
        }

        auto* quickItem = qobject_cast<QQuickItem*>(item);
        if (!quickItem) {
            core::logger::error("startAnalyzerCapture: target item is null or not a QQuickItem");
            return false;
        }

        if (m_analyzerRunning) {
            return true;
        }

        QDir outputDir(m_analyzerOutputFolder);
        if (!outputDir.exists()) {
            outputDir.mkpath(".");
        }

        if (m_runtimeConfig.analyzer.saveFrames) {
            outputDir.mkpath("frames");
        }

        if (m_runtimeConfig.analyzer.saveTrace) {
            m_analyzerTraceFile.setFileName(outputDir.filePath("trace.csv"));
            if (m_analyzerTraceFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream stream(&m_analyzerTraceFile);
                stream << "timestamp_ms,cell_flow,robot_motion,robot_gripped,robot_job,gantry_x,gantry_z,"
                          "gantry_gripped,entry_parts,exit_parts,transfer_parts,entry_damper,exit_damper\n";
                stream.flush();
            }
            else {
                core::logger::error("Failed to open analyzer trace file at {}",
                                    m_analyzerTraceFile.fileName().toStdString());
            }
        }

        m_analyzerTargetItem = quickItem;
        m_analyzerCapturedFrames = 0;
        m_analyzerFrameAccumulator = 0.0;
        m_analyzerTraceAccumulator = 0.0;
        m_analyzerFrameCapturePending = false;
        m_analyzerRunning = true;
        m_analyzerElapsed.restart();
        emit analyzerCaptureRunningChanged();
        emit analyzerCaptureProgressChanged();
        return true;
    }

    void Backend::stopAnalyzerCapture()
    {
        if (!m_analyzerRunning) {
            return;
        }

        m_analyzerRunning = false;
        m_analyzerFrameCapturePending = false;
        m_analyzerTargetItem = nullptr;

        if (m_analyzerTraceFile.isOpen()) {
            m_analyzerTraceFile.close();
        }

        emit analyzerCaptureRunningChanged();
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

    void Backend::updateAnalyzer(double deltaTimeSeconds)
    {
        if (!m_analyzerRunning) {
            return;
        }

        m_analyzerFrameAccumulator += deltaTimeSeconds;
        m_analyzerTraceAccumulator += deltaTimeSeconds;

        if (m_runtimeConfig.analyzer.saveTrace &&
            m_analyzerTraceAccumulator * 1000.0 >= m_runtimeConfig.analyzer.traceIntervalMs) {
            m_analyzerTraceAccumulator = 0.0;
            writeAnalyzerTraceSample();
        }

        if (!m_runtimeConfig.analyzer.saveFrames || m_analyzerFrameCapturePending || !m_analyzerTargetItem) {
            return;
        }

        if (m_analyzerFrameAccumulator * 1000.0 < m_runtimeConfig.analyzer.frameIntervalMs) {
            return;
        }
        m_analyzerFrameAccumulator = 0.0;
        m_analyzerFrameCapturePending = true;

        auto outputDir = QDir(m_analyzerOutputFolder);
        const auto frameIndex = m_analyzerCapturedFrames;
        const auto framePath =
          outputDir.filePath(QString("frames/frame_%1.png").arg(frameIndex, 6, 10, QLatin1Char('0')));

        auto result = m_analyzerTargetItem->grabToImage();
        if (!result) {
            m_analyzerFrameCapturePending = false;
            return;
        }

        connect(result.data(), &QQuickItemGrabResult::ready, this, [this, result, framePath]() {
            const bool saved = result->saveToFile(framePath);
            if (!saved) {
                core::logger::error("Failed to save analyzer frame to {}", framePath.toStdString());
            }
            m_analyzerCapturedFrames += saved ? 1 : 0;
            m_analyzerFrameCapturePending = false;
            emit analyzerCaptureProgressChanged();

            if (m_runtimeConfig.analyzer.maxFrames > 0 &&
                m_analyzerCapturedFrames >= m_runtimeConfig.analyzer.maxFrames) {
                stopAnalyzerCapture();
            }
        }, Qt::SingleShotConnection);
    }

    void Backend::writeAnalyzerTraceSample()
    {
        if (!m_analyzerTraceFile.isOpen()) {
            return;
        }

        const qint64 timestampMs = m_analyzerElapsed.isValid() ? m_analyzerElapsed.elapsed() : 0;
        const auto robotStatus = m_robotSim ? m_robotSim->status() : core::sim::RobotStatus{};
        const auto robotGripped = m_robotSim ? m_robotSim->isGripperGripped() : false;
        const auto gantryX = m_gantrySim ? m_gantrySim->xPos() : 0.0;
        const auto gantryZ = m_gantrySim ? m_gantrySim->zPos() : 0.0;
        const auto gantryGripped = m_gantrySim ? m_gantrySim->gripperGripped() : false;
        const auto entryParts = m_entryConveyorSim ? m_entryConveyorSim->parts().size() : 0;
        const auto exitParts = m_exitConveyorSim ? m_exitConveyorSim->parts().size() : 0;
        const auto transferParts = m_transferConveyorSim ? m_transferConveyorSim->parts().size() : 0;
        const auto entryDamper = m_entryConveyorSim ? m_entryConveyorSim->damperOpen() : false;
        const auto exitDamper = m_transferConveyorSim ? m_transferConveyorSim->damperOpen() : false;

        QTextStream stream(&m_analyzerTraceFile);
        stream << timestampMs << "," << internalCellFlowStatus() << ","
               << static_cast<int>(robotStatus.bInMotion) << "," << static_cast<int>(robotGripped) << ","
               << robotStatus.nJobIdFeedback << "," << gantryX << "," << gantryZ << ","
               << static_cast<int>(gantryGripped) << "," << entryParts << "," << exitParts << ","
               << transferParts << "," << static_cast<int>(entryDamper) << "," << static_cast<int>(exitDamper)
               << "\n";
        stream.flush();
    }

    QString Backend::resolveAnalyzerOutputFolder() const
    {
        if (m_runtimeConfig.analyzer.outputFolder.isEmpty()) {
            return QStringLiteral("analysis/session");
        }

        QFileInfo info(m_runtimeConfig.analyzer.outputFolder);
        if (info.isAbsolute()) {
            return info.absoluteFilePath();
        }

        return QDir::current().filePath(m_runtimeConfig.analyzer.outputFolder);
    }

    void Backend::clearAnalyzerArtifactsOnStartup() const
    {
        QDir outputDir(m_analyzerOutputFolder);
        if (!outputDir.exists()) {
            outputDir.mkpath(".");
        }

        QDir framesDir(outputDir.filePath("frames"));
        if (framesDir.exists()) {
            framesDir.removeRecursively();
        }

        const QString tracePath = outputDir.filePath("trace.csv");
        if (QFileInfo::exists(tracePath)) {
            QFile::remove(tracePath);
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
