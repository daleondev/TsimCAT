#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QCoroTask>

class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString welcomeMessage READ welcomeMessage CONSTANT)
    Q_PROPERTY(QString asyncTestStatus READ asyncTestStatus NOTIFY asyncTestStatusChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    QString welcomeMessage() const;
    QString asyncTestStatus() const;

    Q_INVOKABLE void runAsyncTest();

signals:
    void asyncTestStatusChanged();

private:
    QCoro::Task<void> doAsyncTest();
    QString m_asyncTestStatus = "Ready";
};

#endif // BACKEND_H
