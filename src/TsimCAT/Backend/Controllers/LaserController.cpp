#include "LaserController.h"
#include "Link/LinkFactory.hpp"
#include "Link/Raw/IRawLink.hpp"
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
        
        core::link::LinkConfig config{};
        config.port = 12345;
        auto createRes = core::link::create(core::link::Role::Server, core::link::Mode::Raw, core::link::Protocol::Tcp, config);

        if (!createRes) {
            setTcpStatus("Create Failed");
            core::logger::error("Failed to create TCP Server link: {}", createRes.error().message());
            co_return;
        }

        m_link = std::move(*createRes);

        auto* server = m_link->asServer();
        if (!server) {
            setTcpStatus("Invalid Link Type");
            co_return;
        }

        auto startRes = server->start();
        if (!startRes) {
            auto err = QStringLiteral("Start Failed: %1").arg(startRes.error().value());
            setTcpStatus(err);
            core::logger::error("TCP Server start failed: {}", startRes.error().value());
            co_return;
        }

        setTcpStatus("Listening on Port 12345");
        core::logger::info("TCP Server listening on port 12345");

        while(true) {
            auto acceptRes = co_await server->accept();
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
                
                auto* rawLink = m_link->asRaw();
                if (!rawLink) break;

                auto readRes = co_await rawLink->receiveInto("", buffer);
                
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
