#include "Backend.h"
#include <QCoroTimer>
#include <chrono>

using namespace std::chrono_literals;

Backend::Backend(QObject *parent) : QObject(parent)
{
}

QString Backend::welcomeMessage() const
{
    return QStringLiteral("Hello from C++ Backend!");
}

QString Backend::asyncTestStatus() const
{
    return m_asyncTestStatus;
}

void Backend::runAsyncTest()
{
    // Start the coroutine but don't block
    doAsyncTest();
}

QCoro::Task<void> Backend::doAsyncTest()
{
    m_asyncTestStatus = "Testing QCoro... (Waiting 2s)";
    emit asyncTestStatusChanged();

    // Use QCoro to wait without blocking the event loop
    co_await QCoro::sleepFor(2s);

    m_asyncTestStatus = "QCoro Works! (Delay finished)";
    emit asyncTestStatusChanged();
    
    co_return;
}
