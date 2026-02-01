#include "Backend.h"
#include "Link/Raw/TcpServer.hpp"
#include <QCoroTimer>
#include <chrono>
#include <array>

using namespace std::chrono_literals;

Backend::Backend(QObject *parent) : QObject(parent)
{
}

Backend::~Backend() = default;

QString Backend::welcomeMessage() const
{
    return QStringLiteral("Hello from C++ Backend!");
}

QString Backend::asyncTestStatus() const
{
    return m_asyncTestStatus;
}

QString Backend::tcpStatus() const
{
    return m_tcpStatus;
}

QString Backend::lastMessage() const
{
    return m_lastMessage;
}

void Backend::runAsyncTest()
{
    // Start the coroutine but don't block
    doAsyncTest();
}

void Backend::startTcpServer()
{
    runTcpServerLoop();
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

QCoro::Task<void> Backend::runTcpServerLoop()
{
    m_server = std::make_unique<core::link::raw::TcpServer>(12345);
    auto startRes = m_server->start();
    if (!startRes) {
        setTcpStatus(QStringLiteral("Start Failed: %1").arg(startRes.error().value()));
        co_return;
    }

    setTcpStatus("Listening on Port 12345");

    while(true) {
        auto acceptRes = co_await m_server->accept();
        if (!acceptRes) {
            setTcpStatus(QStringLiteral("Accept Failed: %1").arg(QString::fromStdString(acceptRes.error().message())));
            co_await QCoro::sleepFor(1s);
            continue;
        }

        setTcpStatus("Client Connected");

        while(true) {
            std::array<std::byte, 1024> buffer;
            auto readRes = co_await m_server->receiveInto("", buffer);
            
            if (!readRes) {
                setTcpStatus("Client Disconnected");
                break; 
            }

            size_t bytes = *readRes;
            if (bytes > 0) {
                 std::string s(reinterpret_cast<const char*>(buffer.data()), bytes);
                 setLastMessage(QString::fromStdString(s));
            }
        }
    }
}

void Backend::setTcpStatus(const QString& status)
{
    QMetaObject::invokeMethod(this, [this, status](){
        if (m_tcpStatus != status) {
            m_tcpStatus = status;
            emit tcpStatusChanged();
        }
    });
}

void Backend::setLastMessage(const QString& msg)
{
    QMetaObject::invokeMethod(this, [this, msg](){
        if (m_lastMessage != msg) {
            m_lastMessage = msg;
            emit lastMessageChanged();
        }
    });
}