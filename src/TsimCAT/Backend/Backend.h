#ifndef BACKEND_H
#define BACKEND_H

#include "../Core/Coroutines/coroutine.hpp"
#include "Controllers/LaserController.h"

#include <QCoroTask>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::link { class ILink; }
namespace core::sim { class LaserSimulator; }

namespace backend
{ namespace controllers { class LaserController; } }

namespace backend
{
    class Backend : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString welcomeMessage READ welcomeMessage CONSTANT)
        Q_PROPERTY(QString asyncTestStatus READ asyncTestStatus NOTIFY asyncTestStatusChanged)
        Q_PROPERTY(backend::controllers::LaserController* laser READ laser CONSTANT)

      public:
        explicit Backend(QObject* parent = nullptr);
        ~Backend();

        QString welcomeMessage() const;
        QString asyncTestStatus() const;
        backend::controllers::LaserController* laser() const;

        Q_INVOKABLE void runAsyncTest();
        Q_INVOKABLE void captureScreenshot(QObject* item);

      signals:
        void asyncTestStatusChanged();

      private:
        QCoro::Task<void> doAsyncTest();

        QString m_asyncTestStatus = "Ready";
        std::shared_ptr<core::link::ILink> m_tcpLink;
        std::shared_ptr<core::sim::LaserSimulator> m_laserSim;
        std::unique_ptr<backend::controllers::LaserController> m_laserController;
    };
}

#endif // BACKEND_H