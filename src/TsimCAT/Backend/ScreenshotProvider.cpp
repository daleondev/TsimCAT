#include "ScreenshotProvider.h"

#include "Logger/Logger.hpp"
#include "Simulators/ConveyorSimulator.hpp"
#include "Simulators/RobotSimulator.hpp"
#include "Simulators/RotaryTableSimulator.hpp"
#include "Simulators/SimpleCellCoordinator.hpp"

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickItemGrabResult>

namespace backend
{
    ScreenshotProvider::ScreenshotProvider(SimulatorRefs refs,
                                           const QString& outputFolder,
                                           int autoIntervalMs,
                                           int maxFrames,
                                           QObject* parent)
      : QObject(parent)
      , m_refs(std::move(refs))
      , m_outputFolder(outputFolder)
      , m_maxFrames(maxFrames)
      , m_autoTimer(new QTimer(this))
    {
        m_autoTimer->setInterval(autoIntervalMs);
        connect(m_autoTimer, &QTimer::timeout, this, &ScreenshotProvider::requestCapture);
    }

    ScreenshotProvider::~ScreenshotProvider() = default;

    void ScreenshotProvider::setAutoCaptureEnabled(bool enabled)
    {
        if (m_autoCaptureEnabled == enabled)
            return;
        m_autoCaptureEnabled = enabled;
        if (enabled)
            m_autoTimer->start();
        else
            m_autoTimer->stop();
        emit autoCaptureEnabledChanged();
    }

    void ScreenshotProvider::requestCapture()
    {
        if (m_capturePending)
            return;
        if (m_captureCount >= m_maxFrames && m_maxFrames > 0) {
            core::logger::info("ScreenshotProvider: max frame count ({}) reached, skipping capture",
                               m_maxFrames);
            return;
        }
        m_capturePending = true;
        emit capturePendingChanged();
    }

    void ScreenshotProvider::onFrameCaptured(const QVariant& imageResult)
    {
        m_capturePending = false;
        emit capturePendingChanged();

        if (!ensureOutputFolder())
            return;

        auto image = imageResult.value<QImage>();
        if (image.isNull()) {
            core::logger::error("ScreenshotProvider: received null image from grabToImage");
            return;
        }

        const auto timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
        const auto baseName = QStringLiteral("frame_%1").arg(timestamp);

        const auto pngPath = m_outputFolder + QLatin1Char('/') + baseName + QStringLiteral(".png");
        if (!image.save(pngPath, "PNG")) {
            core::logger::error("ScreenshotProvider: failed to save PNG to {}", pngPath.toStdString());
            return;
        }

        const auto jsonPath = m_outputFolder + QLatin1Char('/') + baseName + QStringLiteral(".json");
        auto stateData = buildStateSnapshot();
        QFile jsonFile(jsonPath);
        if (jsonFile.open(QIODevice::WriteOnly)) {
            jsonFile.write(stateData);
            jsonFile.close();
        }
        else {
            core::logger::error("ScreenshotProvider: failed to write state JSON to {}",
                                jsonPath.toStdString());
        }

        ++m_captureCount;
        emit captureCountChanged();
        core::logger::info(
          "ScreenshotProvider: captured frame {} -> {}", m_captureCount, pngPath.toStdString());
    }

    auto ScreenshotProvider::buildStateSnapshot() const -> QByteArray
    {
        QJsonObject root;
        root[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        root[QStringLiteral("frameIndex")] = m_captureCount;

        // Robot state
        if (m_refs.robot) {
            QJsonObject robot;
            const auto status = m_refs.robot->status();
            const auto control = m_refs.robot->control();
            const double* joints = m_refs.robot->jointAngles();
            const auto pose = m_refs.robot->currentPose();

            QJsonArray jointArr;
            for (int i = 0; i < 6; ++i)
                jointArr.append(joints[i]);
            robot[QStringLiteral("jointAnglesDeg")] = jointArr;

            QJsonObject tcp;
            tcp[QStringLiteral("x")] = pose.x;
            tcp[QStringLiteral("y")] = pose.y;
            tcp[QStringLiteral("z")] = pose.z;
            tcp[QStringLiteral("roll")] = pose.roll;
            tcp[QStringLiteral("pitch")] = pose.pitch;
            tcp[QStringLiteral("yaw")] = pose.yaw;
            robot[QStringLiteral("tcp")] = tcp;

            robot[QStringLiteral("gripperGripped")] = m_refs.robot->isGripperGripped();
            robot[QStringLiteral("inMotion")] = static_cast<bool>(status.bInMotion);
            robot[QStringLiteral("inHome")] = static_cast<bool>(status.bInHome);
            robot[QStringLiteral("enabled")] = static_cast<bool>(status.bEnabled);
            robot[QStringLiteral("error")] = static_cast<bool>(status.bError);
            robot[QStringLiteral("errorCode")] = static_cast<int>(status.nErrorCode);
            robot[QStringLiteral("jobIdFeedback")] = static_cast<int>(status.nJobIdFeedback);
            robot[QStringLiteral("controlJobId")] = static_cast<int>(control.nJobId);
            robot[QStringLiteral("controlMoveEnable")] = static_cast<bool>(control.bMoveEnable);
            robot[QStringLiteral("internalMode")] = m_refs.robot->isInternalMode();
            root[QStringLiteral("robot")] = robot;
        }

        // Rotary table state
        if (m_refs.rotaryTable) {
            QJsonObject table;
            const auto status = m_refs.rotaryTable->status();
            table[QStringLiteral("angleDeg")] = static_cast<double>(status.fAngleDeg);
            table[QStringLiteral("partPresent")] = static_cast<bool>(status.bPartPresent);
            table[QStringLiteral("readyToPick")] = static_cast<bool>(status.bReadyToPick);
            table[QStringLiteral("atLoadPosition")] = static_cast<bool>(status.bAtLoadPosition);
            table[QStringLiteral("atPickPosition")] = static_cast<bool>(status.bAtPickPosition);
            table[QStringLiteral("busy")] = static_cast<bool>(status.bBusy);
            table[QStringLiteral("internalMode")] = m_refs.rotaryTable->isInternalMode();
            root[QStringLiteral("rotaryTable")] = table;
        }

        // Exit conveyor state
        if (m_refs.exitConveyor) {
            QJsonObject conveyor;
            conveyor[QStringLiteral("isRunning")] = m_refs.exitConveyor->isRunning();
            conveyor[QStringLiteral("speed")] = m_refs.exitConveyor->speed();
            conveyor[QStringLiteral("damperOpen")] = m_refs.exitConveyor->damperOpen();
            conveyor[QStringLiteral("autoLogic")] = m_refs.exitConveyor->autoLogic();
            conveyor[QStringLiteral("autoSpawn")] = m_refs.exitConveyor->autoSpawn();
            conveyor[QStringLiteral("internalMode")] = m_refs.exitConveyor->isInternalMode();

            auto partsList = m_refs.exitConveyor->parts();
            QJsonArray partsArr;
            for (const auto& part : partsList) {
                QJsonObject p;
                p[QStringLiteral("id")] = static_cast<int>(part.id);
                p[QStringLiteral("type")] = static_cast<int>(part.type);
                p[QStringLiteral("position")] = part.position;
                p[QStringLiteral("width")] = part.width;
                p[QStringLiteral("length")] = part.length;
                p[QStringLiteral("height")] = part.height;
                partsArr.append(p);
            }
            conveyor[QStringLiteral("parts")] = partsArr;

            auto sensorStates = m_refs.exitConveyor->sensors();
            QJsonArray sensorArr;
            for (bool s : sensorStates)
                sensorArr.append(s);
            conveyor[QStringLiteral("sensors")] = sensorArr;

            root[QStringLiteral("exitConveyor")] = conveyor;
        }

        // Coordinator state
        if (m_refs.coordinator) {
            QJsonObject coord;
            coord[QStringLiteral("enabled")] = m_refs.coordinator->enabled();
            coord[QStringLiteral("laserStationHasPart")] = m_refs.coordinator->laserStationHasPart();
            coord[QStringLiteral("tableSimEnabled")] = m_refs.coordinator->tableSimulationEnabled();
            coord[QStringLiteral("robotSimEnabled")] = m_refs.coordinator->robotSimulationEnabled();
            coord[QStringLiteral("laserSimEnabled")] = m_refs.coordinator->laserSimulationEnabled();
            coord[QStringLiteral("conveyorSimEnabled")] = m_refs.coordinator->conveyorSimulationEnabled();
            coord[QStringLiteral("autoSpawnParts")] = m_refs.coordinator->autoSpawnParts();
            root[QStringLiteral("coordinator")] = coord;
        }

        return QJsonDocument(root).toJson(QJsonDocument::Indented);
    }

    auto ScreenshotProvider::ensureOutputFolder() -> bool
    {
        QDir dir(m_outputFolder);
        if (!dir.exists()) {
            if (!dir.mkpath(QStringLiteral("."))) {
                core::logger::error("ScreenshotProvider: failed to create output directory: {}",
                                    m_outputFolder.toStdString());
                return false;
            }
        }
        return true;
    }
}
