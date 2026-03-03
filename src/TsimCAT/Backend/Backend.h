#ifndef BACKEND_H
#define BACKEND_H

#include "../Core/Coroutines/coroutine.hpp"
#include "Controllers/ConveyorController.h"
#include "Controllers/RobotController.h"
#include "RuntimeConfig.h"

#include <QCoroTask>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::link
{
    class ILink;
}
namespace core::sim
{
    class RobotSimulator;
    class ConveyorSimulator;
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
        Q_PROPERTY(backend::controllers::ConveyorController* entryConveyor READ entryConveyor CONSTANT)
        Q_PROPERTY(backend::controllers::ConveyorController* exitConveyor READ exitConveyor CONSTANT)
        Q_PROPERTY(bool localRobotMode READ localRobotMode WRITE setLocalRobotMode NOTIFY stationModesChanged)
        Q_PROPERTY(bool localEntryConveyorMode READ localEntryConveyorMode WRITE setLocalEntryConveyorMode
                     NOTIFY stationModesChanged)
        Q_PROPERTY(bool localExitConveyorMode READ localExitConveyorMode WRITE setLocalExitConveyorMode NOTIFY
                     stationModesChanged)

      public:
        explicit Backend(QObject* parent = nullptr);
        ~Backend();

        QString welcomeMessage() const;
        QString asyncTestStatus() const;
        backend::controllers::RobotController* robot() const;
        backend::controllers::ConveyorController* entryConveyor() const;
        backend::controllers::ConveyorController* exitConveyor() const;
        bool localRobotMode() const;
        bool localEntryConveyorMode() const;
        bool localExitConveyorMode() const;
        void setLocalRobotMode(bool enabled);
        void setLocalEntryConveyorMode(bool enabled);
        void setLocalExitConveyorMode(bool enabled);

        Q_INVOKABLE void runAsyncTest();

      signals:
        void asyncTestStatusChanged();
        void stationModesChanged();

      private:
        QCoro::Task<void> doAsyncTest();
        void ensureRobotCommTask();
        void ensureEntryConveyorCommTask();
        void ensureExitConveyorCommTask();

        QString m_asyncTestStatus = "Ready";
        RuntimeConfig m_runtimeConfig;
        std::shared_ptr<core::link::ILink> m_tcpLink;
        std::shared_ptr<core::link::ILink> m_adsLink;

        std::shared_ptr<core::sim::RobotSimulator> m_robotSim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_entryConveyorSim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_exitConveyorSim;

        std::unique_ptr<backend::controllers::RobotController> m_robotController;
        std::unique_ptr<backend::controllers::ConveyorController> m_entryConveyorController;
        std::unique_ptr<backend::controllers::ConveyorController> m_exitConveyorController;

        bool m_robotCommTaskStarted{ false };
        bool m_entryConveyorCommTaskStarted{ false };
        bool m_exitConveyorCommTaskStarted{ false };
    };
}

#endif // BACKEND_H
