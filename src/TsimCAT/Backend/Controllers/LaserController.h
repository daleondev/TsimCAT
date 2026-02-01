#pragma once

#include "../../Core/Coroutines/coroutine.hpp"

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QCoroTask>
#include <memory>

namespace core::sim { class LaserSimulator; }

namespace backend::controllers
{
    class LaserController : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString tcpStatus READ tcpStatus NOTIFY tcpStatusChanged)
        Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

      public:
        explicit LaserController(std::shared_ptr<core::sim::LaserSimulator> simulator, QObject* parent = nullptr);
        ~LaserController();

        QString tcpStatus() const;
        QString lastMessage() const;

        Q_INVOKABLE void startTcpServer();

      signals:
        void tcpStatusChanged();
        void lastMessageChanged();

      private:
        std::shared_ptr<core::sim::LaserSimulator> m_simulator;
    };
}
