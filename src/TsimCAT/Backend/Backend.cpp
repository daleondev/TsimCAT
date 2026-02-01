#include "Backend.h"
#include "Link/Raw/TcpServer.hpp"
#include "Logger/Logger.hpp"
#include <QCoroTimer>
#include <chrono>
#include <array>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QDir>
#include <QDateTime>

using namespace std::chrono_literals;

Backend::Backend(QObject *parent) : QObject(parent)
{
    // Initialize logger
    core::logger::Logger::instance().init("logs/TsimCAT.log");
    core::logger::info("Backend initialized");
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

void Backend::captureScreenshot(QObject* item)
{
    auto* quickItem = qobject_cast<QQuickItem*>(item);
    if (!quickItem) {
        core::logger::error("captureScreenshot: Item is null or not a QQuickItem");
        return;
    }

    auto result = quickItem->grabToImage();
    if (result) {
        connect(result.data(), &QQuickItemGrabResult::ready, this, [result]() {
            QDir dir("screenshots");
            if (!dir.exists()) dir.mkpath(".");
            
            QString filename = QString("screenshots/capture_%1.png")
                               .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));
            if (result->saveToFile(filename)) {
                core::logger::info("Screenshot saved to {}", filename.toStdString());
            } else {
                core::logger::error("Failed to save screenshot");
            }
        });
    }
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
    core::logger::info("Starting TCP Server loop");
    m_server = std::make_unique<core::link::raw::TcpServer>(12345);
    auto startRes = m_server->start();
    if (!startRes) {
        auto err = QStringLiteral("Start Failed: %1").arg(startRes.error().value());
        setTcpStatus(err);
        core::logger::error("TCP Server start failed: {}", startRes.error().value());
        co_return;
    }

    setTcpStatus("Listening on Port 12345");
    core::logger::info("TCP Server listening on port 12345");

    while(true) {
        auto acceptRes = co_await m_server->accept();
        if (!acceptRes) {
            auto msg = acceptRes.error().message();
            setTcpStatus(QStringLiteral("Accept Failed: %1").arg(QString::fromStdString(msg)));
            core::logger::warn("TCP Accept failed: {}", msg);
            co_await QCoro::sleepFor(1s);
            continue;
        }

        setTcpStatus("Client Connected");
        core::logger::info("Client connected");

        while(true) {
            std::array<std::byte, 1024> buffer;
            core::logger::debug("Waiting for data...");
            auto readRes = co_await m_server->receiveInto("", buffer);
            
            if (!readRes) {
                setTcpStatus("Client Disconnected");
                core::logger::info("Client disconnected");
                break; 
            }

            size_t bytes = *readRes;
            if (bytes > 0) {
                 std::string s(reinterpret_cast<const char*>(buffer.data()), bytes);
                 
                 // Hex dump for debugging
                 std::string hex;
                 for (auto c : s) hex += std::format("{:02X} ", static_cast<unsigned char>(c));
                 core::logger::debug("Raw RX ({} bytes): [ {}] Text: '{}'", bytes, hex, s);

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
        core::logger::debug("Update Request - Old: '{}' New: '{}'", m_lastMessage.toStdString(), msg.toStdString());
        
        if (m_lastMessage != msg) {
            m_lastMessage = msg;
            emit lastMessageChanged();
            core::logger::info("Property 'lastMessage' updated");
        } else {
            core::logger::warn("Property 'lastMessage' skipped (Identical content)");
        }
    });
}