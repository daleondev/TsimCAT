#include "Backend.h"

Backend::Backend(QObject *parent) : QObject(parent)
{
}

QString Backend::welcomeMessage() const
{
    return QStringLiteral("Hello from C++ Backend!");
}
