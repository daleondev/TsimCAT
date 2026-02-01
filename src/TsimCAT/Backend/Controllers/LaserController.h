#pragma once

#include "../../Core/Coroutines/coroutine.hpp"

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QCoroTask>
#include <memory>

namespace core::link { class ILink; }

namespace backend::controllers
{
    class LaserController : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QString tcpStatus READ tcpStatus NOTIFY tcpStatusChanged)
        Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

      public:
        explicit LaserController(QObject* parent = nullptr);
        ~LaserController();

        QString tcpStatus() const;
        QString lastMessage() const;

        Q_INVOKABLE void startTcpServer();

      signals:
        void tcpStatusChanged();
        void lastMessageChanged();

      private:
        QCoro::Task<void> runTcpServerLoop();
        void setTcpStatus(const QString& status);
        void setLastMessage(const QString& msg);

        QString m_tcpStatus = "Disconnected";
        QString m_lastMessage = "No messages";

        std::unique_ptr<core::link::ILink> m_link;
    };
}
