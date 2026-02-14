#ifndef BACKEND_H
#define BACKEND_H

#include "../Core/Coroutines/coroutine.hpp"
#include "Controllers/LaserController.h"
#include "Controllers/RobotController.h"
#include "Controllers/ConveyorController.h"

#include <QCoroTask>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::link { class ILink; }
namespace core::sim { class LaserSimulator; class RobotSimulator; class ConveyorSimulator; }

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
        Q_PROPERTY(backend::controllers::ConveyorController* entryConveyor READ entryConveyor CONSTANT)
        Q_PROPERTY(backend::controllers::ConveyorController* exitConveyor READ exitConveyor CONSTANT)

      public:
        explicit Backend(QObject* parent = nullptr);
        ~Backend();

        QString welcomeMessage() const;
        QString asyncTestStatus() const;
        backend::controllers::LaserController* laser() const;
        backend::controllers::RobotController* robot() const;
        backend::controllers::ConveyorController* entryConveyor() const;
        backend::controllers::ConveyorController* exitConveyor() const;

        Q_INVOKABLE void runAsyncTest();
        Q_INVOKABLE void captureScreenshot(QObject* item, const QString& filename = QString());

      signals:
        void asyncTestStatusChanged();

      private:
        QCoro::Task<void> doAsyncTest();

        QString m_asyncTestStatus = "Ready";
        std::shared_ptr<core::link::ILink> m_tcpLink;
        std::shared_ptr<core::link::ILink> m_adsLink;

        std::shared_ptr<core::sim::LaserSimulator> m_laserSim;
        std::shared_ptr<core::sim::RobotSimulator> m_robotSim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_entryConveyorSim;
        std::shared_ptr<core::sim::ConveyorSimulator> m_exitConveyorSim;

        std::unique_ptr<backend::controllers::LaserController> m_laserController;
        std::unique_ptr<backend::controllers::RobotController> m_robotController;
        std::unique_ptr<backend::controllers::ConveyorController> m_entryConveyorController;
        std::unique_ptr<backend::controllers::ConveyorController> m_exitConveyorController;
    };
}

#endif // BACKEND_H