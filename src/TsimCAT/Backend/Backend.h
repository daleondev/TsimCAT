#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString welcomeMessage READ welcomeMessage CONSTANT)

public:
    explicit Backend(QObject *parent = nullptr);
    QString welcomeMessage() const;
};

#endif // BACKEND_H