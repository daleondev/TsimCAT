#include "LaserController.h"
#include "Logger/Logger.hpp"
#include <QCoroTimer>
#include <array>

using namespace std::chrono_literals;

namespace backend::controllers
{
    LaserController::LaserController(QObject* parent)
      : QObject(parent)
    {
    }

    LaserController::~LaserController() = default;

    QString LaserController::tcpStatus() const
    {
        return m_tcpStatus;
    }

    QString LaserController::lastMessage() const
    {
        return m_lastMessage;
    }

    void LaserController::startTcpServer()
    {
        runTcpServerLoop();
    }

    QCoro::Task<void> LaserController::runTcpServerLoop()
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
                     
                     std::string hex;
                     for (auto c : s) hex += std::format("{:02X} ", static_cast<unsigned char>(c));
                     core::logger::debug("Raw RX ({} bytes): [ {}] Text: '{}'", bytes, hex, s);

                     setLastMessage(QString::fromStdString(s));
                }
            }
        }
    }

    void LaserController::setTcpStatus(const QString& status)
    {
        QMetaObject::invokeMethod(this, [this, status](){
            if (m_tcpStatus != status) {
                m_tcpStatus = status;
                emit tcpStatusChanged();
            }
        });
    }

    void LaserController::setLastMessage(const QString& msg)
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
}
