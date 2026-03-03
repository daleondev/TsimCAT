#ifndef BACKEND_H
#define BACKEND_H

#include "../Core/Coroutines/coroutine.hpp"
#include "Controllers/ConveyorController.h"
#include "Controllers/GantryController.h"
#include "Controllers/LaserController.h"
#include "Controllers/RobotController.h"
#include "RuntimeConfig.h"

#include <QCoroTask>
#include <QElapsedTimer>
#include <QFile>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::link
{
    class ILink;
}
namespace core::sim
{
    class LaserSimulator;
    class RobotSimulator;
    class ConveyorSimulator;
    class GantrySimulator;
    class CellFlowOrchestrator;
}

namespace backend
{
    class Backend : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString welcomeMessage READ welcomeMessage CONSTANT)
        Q_PROPERTY(QString asyncTestStatus READ asyncTestStatus NOTIFY asyncTestStatusChanged)
        Q_PROPERTY(backend::controllers::LaserController* laser READ laser CONSTANT)
        Q_PROPERTY(backend::controllers::RobotController* robot READ robot CONSTANT)
        Q_PROPERTY(backend::controllers::GantryController* gantry READ gantry CONSTANT)
        Q_PROPERTY(backend::controllers::ConveyorController* entryConveyor READ entryConveyor CONSTANT)
        Q_PROPERTY(backend::controllers::ConveyorController* exitConveyor READ exitConveyor CONSTANT)
        Q_PROPERTY(backend::controllers::ConveyorController* transferConveyor READ transferConveyor CONSTANT)
        Q_PROPERTY(
          bool internalCellFlowRunning READ internalCellFlowRunning NOTIFY internalCellFlowRunningChanged)
        Q_PROPERTY(
          QString internalCellFlowStatus READ internalCellFlowStatus NOTIFY internalCellFlowStatusChanged)
        Q_PROPERTY(bool localRobotMode READ localRobotMode WRITE setLocalRobotMode NOTIFY stationModesChanged)
        Q_PROPERTY(
          bool localCameraMode READ localCameraMode WRITE setLocalCameraMode NOTIFY stationModesChanged)
        Q_PROPERTY(bool localLaserMode READ localLaserMode WRITE setLocalLaserMode NOTIFY stationModesChanged)
        Q_PROPERTY(
          bool localGantryMode READ localGantryMode WRITE setLocalGantryMode NOTIFY stationModesChanged)
        Q_PROPERTY(bool localEntryConveyorMode READ localEntryConveyorMode WRITE setLocalEntryConveyorMode
                     NOTIFY stationModesChanged)
        Q_PROPERTY(bool localExitConveyorMode READ localExitConveyorMode WRITE setLocalExitConveyorMode NOTIFY
                     stationModesChanged)
        Q_PROPERTY(bool robotCarriedPartVisible READ robotCarriedPartVisible NOTIFY partVisualizationChanged)
        Q_PROPERTY(int robotCarriedPartType READ robotCarriedPartType NOTIFY partVisualizationChanged)
        Q_PROPERTY(bool cameraPartVisible READ cameraPartVisible NOTIFY partVisualizationChanged)
        Q_PROPERTY(int cameraPartType READ cameraPartType NOTIFY partVisualizationChanged)
        Q_PROPERTY(bool laserPartVisible READ laserPartVisible NOTIFY partVisualizationChanged)
        Q_PROPERTY(int laserPartType READ laserPartType NOTIFY partVisualizationChanged)
        Q_PROPERTY(int rejectBinCount READ rejectBinCount NOTIFY partVisualizationChanged)
        Q_PROPERTY(bool analyzerEnabled READ analyzerEnabled CONSTANT)
        Q_PROPERTY(
          bool analyzerCaptureRunning READ analyzerCaptureRunning NOTIFY analyzerCaptureRunningChanged)
        Q_PROPERTY(
          int analyzerCapturedFrames READ analyzerCapturedFrames NOTIFY analyzerCaptureProgressChanged)
        Q_PROPERTY(QString analyzerOutputFolder READ analyzerOutputFolder CONSTANT)

      public:
        explicit Backend(QObject* parent = nullptr);
        ~Backend();

        QString welcomeMessage() const;
        QString asyncTestStatus() const;
        backend::controllers::LaserController* laser() const;
        backend::controllers::RobotController* robot() const;
        backend::controllers::GantryController* gantry() const;
        backend::controllers::ConveyorController* entryConveyor() const;
        backend::controllers::ConveyorController* exitConveyor() const;
        backend::controllers::ConveyorController* transferConveyor() const;
        bool internalCellFlowRunning() const;
        QString internalCellFlowStatus() const;
        bool localRobotMode() const;
        bool localCameraMode() const;
        bool localLaserMode() const;
        bool localGantryMode() const;
        bool localEntryConveyorMode() const;
        bool localExitConveyorMode() const;
        bool robotCarriedPartVisible() const;
        int robotCarriedPartType() const;
        bool cameraPartVisible() const;
        int cameraPartType() const;
        bool laserPartVisible() const;
        int laserPartType() const;
        int rejectBinCount() const;
        bool analyzerEnabled() const;
        bool analyzerCaptureRunning() const;
        int analyzerCapturedFrames() const;
        QString analyzerOutputFolder() const;
        void setLocalRobotMode(bool enabled);
        void setLocalCameraMode(bool enabled);
        void setLocalLaserMode(bool enabled);
        void setLocalGantryMode(bool enabled);
        void setLocalEntryConveyorMode(bool enabled);
        void setLocalExitConveyorMode(bool enabled);

        Q_INVOKABLE void runAsyncTest();
        Q_INVOKABLE void captureScreenshot(QObject* item, const QString& filename = QString());
        Q_INVOKABLE void startInternalCellFlow();
        Q_INVOKABLE void stopInternalCellFlow();
        Q_INVOKABLE bool setStationLocalMode(const QString& station, bool enabled);
        Q_INVOKABLE bool startAnalyzerCapture(QObject* item);
        Q_INVOKABLE void stopAnalyzerCapture();

      signals:
        void asyncTestStatusChanged();
        void internalCellFlowRunningChanged();
        void internalCellFlowStatusChanged();
        void stationModesChanged();
        void partVisualizationChanged();
        void analyzerCaptureRunningChanged();
        void analyzerCaptureProgressChanged();

      private:
        QCoro::Task<void> doAsyncTest();

        QString m_asyncTestStatus = "Ready";
        RuntimeConfig m_runtimeConfig;
        std::shared_ptr<core::link::ILink> m_tcpLink;
        std::shared_ptr<core::link::ILink> m_adsLink;

        std::shared_ptr<core::sim::LaserSimulator> m_laserSim;
        std::shared_ptr<core::sim::RobotSimulator> m_robotSim;
        std::shared_ptr<core::sim::GantrySimulator> m_gantrySim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_entryConveyorSim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_exitConveyorSim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_transferConveyorSim;
        std::unique_ptr<core::sim::CellFlowOrchestrator> m_cellFlow;

        std::unique_ptr<backend::controllers::LaserController> m_laserController;
        std::unique_ptr<backend::controllers::RobotController> m_robotController;
        std::unique_ptr<backend::controllers::GantryController> m_gantryController;
        std::unique_ptr<backend::controllers::ConveyorController> m_entryConveyorController;
        std::unique_ptr<backend::controllers::ConveyorController> m_exitConveyorController;
        std::unique_ptr<backend::controllers::ConveyorController> m_transferConveyorController;

        bool m_internalCellFlowRunning{ false };
        QString m_internalCellFlowStatus{ "Idle" };
        bool m_robotCommTaskStarted{ false };
        bool m_laserCommTaskStarted{ false };
        bool m_entryConveyorCommTaskStarted{ false };
        bool m_exitConveyorCommTaskStarted{ false };

        bool m_analyzerRunning{ false };
        bool m_analyzerFrameCapturePending{ false };
        int m_analyzerCapturedFrames{ 0 };
        double m_analyzerFrameAccumulator{ 0.0 };
        double m_analyzerTraceAccumulator{ 0.0 };
        QString m_analyzerOutputFolder;
        QPointer<QQuickItem> m_analyzerTargetItem;
        QFile m_analyzerTraceFile;
        QElapsedTimer m_analyzerElapsed;

        void ensureRobotCommTask();
        void ensureLaserCommTask();
        void ensureEntryConveyorCommTask();
        void ensureExitConveyorCommTask();
        bool allStationsLocal() const;
        void resyncInternalCellFlowOnLocalReenable(const char* stationName);
        void updateCellFlowStatusText();
        void updateAnalyzer(double deltaTimeSeconds);
        void writeAnalyzerTraceSample();
        QString resolveAnalyzerOutputFolder() const;
        void clearAnalyzerArtifactsOnStartup() const;
    };
}

#endif // BACKEND_H