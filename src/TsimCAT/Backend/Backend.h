#ifndef BACKEND_H
#define BACKEND_H

#include "../Core/Coroutines/Task.hpp"
#include "Controllers/ConveyorController.h"
#include "Controllers/RobotController.h"
#include "Controllers/RotaryTableController.h"
#include "RuntimeConfig.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <chrono>
#include <memory>
#include <vector>

namespace core::link
{
    class ILink;
}
namespace core::sim
{
    class RobotSimulator;
    class ConveyorSimulator;
    class RotaryTableSimulator;
    class SimpleCellCoordinator;
}

namespace backend
{
    class Backend : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString welcomeMessage READ welcomeMessage CONSTANT)
        Q_PROPERTY(QString asyncTestStatus READ asyncTestStatus NOTIFY asyncTestStatusChanged)
        Q_PROPERTY(backend::controllers::RobotController* robot READ robot CONSTANT)
        Q_PROPERTY(backend::controllers::RotaryTableController* rotaryTable READ rotaryTable CONSTANT)
        Q_PROPERTY(backend::controllers::ConveyorController* exitConveyor READ exitConveyor CONSTANT)
        Q_PROPERTY(bool robotCarriedPartVisible READ robotCarriedPartVisible NOTIFY partVisualizationChanged)
        Q_PROPERTY(int robotCarriedPartType READ robotCarriedPartType NOTIFY partVisualizationChanged)
        Q_PROPERTY(bool laserPartVisible READ laserPartVisible NOTIFY partVisualizationChanged)
        Q_PROPERTY(int laserPartType READ laserPartType NOTIFY partVisualizationChanged)
        Q_PROPERTY(bool usingLocalAdsShadow READ usingLocalAdsShadow CONSTANT)
        Q_PROPERTY(bool localPlcShadow READ localPlcShadow CONSTANT)
        Q_PROPERTY(bool localRobotMode READ localRobotMode WRITE setLocalRobotMode NOTIFY stationModesChanged)
        Q_PROPERTY(bool localRotaryTableMode READ localRotaryTableMode WRITE setLocalRotaryTableMode NOTIFY
                     stationModesChanged)
        Q_PROPERTY(bool localExitConveyorMode READ localExitConveyorMode WRITE setLocalExitConveyorMode NOTIFY
                     stationModesChanged)

      public:
        explicit Backend(QObject* parent = nullptr);
        ~Backend();

        QString welcomeMessage() const;
        QString asyncTestStatus() const;
        backend::controllers::RobotController* robot() const;
        backend::controllers::RotaryTableController* rotaryTable() const;
        backend::controllers::ConveyorController* exitConveyor() const;
        bool robotCarriedPartVisible() const;
        int robotCarriedPartType() const;
        bool laserPartVisible() const;
        int laserPartType() const;
        bool usingLocalAdsShadow() const;
        bool localPlcShadow() const;
        bool localRobotMode() const;
        bool localRotaryTableMode() const;
        bool localExitConveyorMode() const;
        void setLocalRobotMode(bool enabled);
        void setLocalRotaryTableMode(bool enabled);
        void setLocalExitConveyorMode(bool enabled);

        Q_INVOKABLE void runAsyncTest();

      signals:
        void asyncTestStatusChanged();
        void stationModesChanged();
        void partVisualizationChanged();

      private:
        void startBackgroundTask(core::coro::Task<void>&& task);
        void startUpdateLoop();
        void ensureRobotCommTask();
        void ensureRotaryTableCommTask();
        void ensureExitConveyorCommTask();

        QString m_asyncTestStatus = "Ready";
        RuntimeConfig m_runtimeConfig;
        std::shared_ptr<core::link::ILink> m_tcpLink;
        std::shared_ptr<core::link::ILink> m_adsLink;

        std::shared_ptr<core::sim::RobotSimulator> m_robotSim;
        std::shared_ptr<core::sim::RotaryTableSimulator> m_rotaryTableSim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_exitConveyorSim;
        std::shared_ptr<core::sim::SimpleCellCoordinator> m_cellCoordinator;

        std::unique_ptr<backend::controllers::RobotController> m_robotController;
        std::unique_ptr<backend::controllers::RotaryTableController> m_rotaryTableController;
        std::unique_ptr<backend::controllers::ConveyorController> m_exitConveyorController;

        bool m_robotCommTaskStarted{ false };
        bool m_rotaryTableCommTaskStarted{ false };
        bool m_exitConveyorCommTaskStarted{ false };
        QTimer* m_updateTimer{ nullptr };
        std::chrono::steady_clock::time_point m_lastUpdateTime{};
        std::vector<core::coro::Task<void>> m_backgroundTasks;
    };
}

#endif // BACKEND_H
