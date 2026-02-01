#ifndef BACKEND_H
#define BACKEND_H

#include "../Core/Coroutines/coroutine.hpp"

#include <QCoroTask>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::link::raw { class TcpServer; }

class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString welcomeMessage READ welcomeMessage CONSTANT)
    Q_PROPERTY(QString asyncTestStatus READ asyncTestStatus NOTIFY asyncTestStatusChanged)
    Q_PROPERTY(QString tcpStatus READ tcpStatus NOTIFY tcpStatusChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

  public:
    explicit Backend(QObject* parent = nullptr);
    ~Backend();

    QString welcomeMessage() const;
    QString asyncTestStatus() const;
    QString tcpStatus() const;
    QString lastMessage() const;

    Q_INVOKABLE void runAsyncTest();
    Q_INVOKABLE void startTcpServer();

  signals:
    void asyncTestStatusChanged();
    void tcpStatusChanged();
    void lastMessageChanged();

  private:
    QCoro::Task<void> doAsyncTest();
    QCoro::Task<void> runTcpServerLoop();
    void setTcpStatus(const QString& status);
    void setLastMessage(const QString& msg);

    QString m_asyncTestStatus = "Ready";
    QString m_tcpStatus = "Disconnected";
    QString m_lastMessage = "No messages";

    std::unique_ptr<core::link::raw::TcpServer> m_server;
};

#endif // BACKEND_H