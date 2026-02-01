#include "LaserSimulator.hpp"
#include "Logger/Logger.hpp"
#include "Link/Raw/IRawLink.hpp"
#include <array>
#include <format>

namespace core::sim
{
    LaserSimulator::LaserSimulator(std::shared_ptr<link::ILink> link)
        : m_link(std::move(link))
    {
    }

    LaserSimulator::~LaserSimulator()
    {
        stop();
    }

    auto LaserSimulator::initialize() -> coro::Task<void>
    {
        if (auto* server = m_link->asServer()) {
            auto res = server->start();
            if (!res) {
                logger::error("LaserSimulator: Failed to start server: {}", res.error().message());
            } else {
                logger::info("LaserSimulator: Server started");
            }
        }
        co_return;
    }

    auto LaserSimulator::start() -> void
    {
        m_running = true;
        // In a real system, we'd use a Task scheduler/executor to fire this.
        // For now, it will be driven by the first await in initialize or similar.
        // We'll let the Controller/Backend manage the Task execution for now.
    }

    auto LaserSimulator::stop() -> void
    {
        m_running = false;
        if (auto* server = m_link->asServer()) {
            server->stop();
        }
    }

    auto LaserSimulator::update(double deltaTimeSeconds) -> void
    {
        // Physics logic would go here
    }

    auto LaserSimulator::tcpStatus() const -> std::string
    {
        if (!m_link) return "No Link";
        switch (m_link->status()) {
            case link::Status::Disconnected: return "Disconnected";
            case link::Status::Connecting: return "Listening";
            case link::Status::Connected: return "Client Connected";
            case link::Status::Faulty: return "Faulty";
            default: return "Unknown";
        }
    }

    auto LaserSimulator::lastMessage() const -> std::string
    {
        std::scoped_lock lock(m_mutex);
        return m_lastMessage;
    }

    auto LaserSimulator::run() -> coro::Task<void>
    {
        auto* server = m_link->asServer();
        auto* raw = m_link->asRaw();
        if (!server || !raw) co_return;

        while (m_running) {
            auto acceptRes = co_await server->accept();
            if (!acceptRes) {
                logger::warn("LaserSimulator: Accept failed: {}", acceptRes.error().message());
                continue;
            }

            while (m_running && m_link->status() == link::Status::Connected) {
                std::array<std::byte, 1024> buffer;
                auto readRes = co_await raw->receiveInto("", buffer);

                if (!readRes) {
                    logger::info("LaserSimulator: Client disconnected");
                    break;
                }

                size_t bytes = *readRes;
                if (bytes > 0) {
                    std::string msg(reinterpret_cast<const char*>(buffer.data()), bytes);
                    {
                        std::scoped_lock lock(m_mutex);
                        m_lastMessage = msg;
                    }
                    logger::debug("LaserSimulator RX: {}", msg);
                }
            }
        }
    }
}
