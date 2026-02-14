#include "LaserSimulator.hpp"
#include "Link/Raw/IRawLink.hpp"
#include "Logger/Logger.hpp"
#include <algorithm>
#include <array>
#include <format>

namespace core::sim
{
    LaserSimulator::LaserSimulator(std::shared_ptr<link::ILink> link)
      : m_link(std::move(link))
    {
    }

    LaserSimulator::~LaserSimulator() { stop(); }

    auto LaserSimulator::initialize() -> coro::Task<result::Result<void>>
    {
        if (!m_link || m_internalMode) {
            co_return result::success();
        }

        if (auto* server = m_link->asServer()) {
            auto res = server->start();
            if (!res) {
                logger::error("LaserSimulator: Failed to start server: {}", res.error().message());
                co_return std::unexpected(res.error());
            }
            else {
                logger::info("LaserSimulator: Server started");
            }
        }
        co_return result::success();
    }

    auto LaserSimulator::start() -> void { m_running = true; }

    auto LaserSimulator::stop() -> void
    {
        m_running = false;
        if (!m_link || m_internalMode) {
            return;
        }
        if (auto* server = m_link->asServer()) {
            server->stop();
        }
    }

    auto LaserSimulator::update(double deltaTimeSeconds) -> void
    {
        std::scoped_lock lock(m_mutex);
        if (m_state == State::Laser) {
            m_workTimer -= deltaTimeSeconds;
            if (m_workTimer <= 0.0) {
                m_state = State::Done;
                m_laserDonePending = true;
                logger::info("LaserSimulator: Job finished");
            }
        }
    }

    auto LaserSimulator::state() const -> State
    {
        std::scoped_lock lock(m_mutex);
        return m_state;
    }

    auto LaserSimulator::stateString() const -> std::string
    {
        switch (state()) {
            case State::Idle:
                return "idle";
            case State::Ready:
                return "ready";
            case State::Pilot:
                return "pilot";
            case State::Laser:
                return "laser";
            case State::Done:
                return "done";
            case State::Error:
                return "error";
            default:
                return "unknown";
        }
    }

    auto LaserSimulator::tcpStatus() const -> std::string
    {
        if (m_internalMode)
            return "Local";
        if (!m_link)
            return "No Link";
        switch (m_link->status()) {
            case link::Status::Disconnected:
                return "Disconnected";
            case link::Status::Connecting:
                return "Listening";
            case link::Status::Connected:
                return "Client Connected";
            case link::Status::Faulty:
                return "Faulty";
            default:
                return "Unknown";
        }
    }

    auto LaserSimulator::lastMessage() const -> std::string
    {
        std::scoped_lock lock(m_mutex);
        return m_lastMessage;
    }

    auto LaserSimulator::run() -> coro::Task<void>
    {
        if (m_internalMode || !m_link)
            co_return;

        auto* server = m_link->asServer();
        auto* raw = m_link->asRaw();
        if (!server || !raw)
            co_return;

        while (m_running) {
            if (isInternalMode()) {
                co_await coro::sleep(std::chrono::milliseconds(100));
                continue;
            }

            auto acceptRes = co_await server->accept();
            if (!acceptRes) {
                logger::warn("LaserSimulator: Accept failed: {}", acceptRes.error().message());
                continue;
            }

            logger::info("LaserSimulator: PLC connected");

            while (m_running && m_link->status() == link::Status::Connected) {
                std::array<std::byte, 1024> buffer;

                // Increased timeout
                auto readRes = co_await raw->receiveInto("", buffer, std::chrono::milliseconds(5000));

                if (readRes) {
                    size_t bytes = *readRes;
                    if (bytes > 0) {
                        std::string msg(reinterpret_cast<const char*>(buffer.data()), bytes);
                        {
                            std::scoped_lock lock(m_mutex);
                            m_lastMessage = msg;
                        }
                        co_await handleCommand(msg);
                    }
                    else {
                        logger::info("LaserSimulator: PLC disconnected (EOF)");
                        break;
                    }
                }
                else if (readRes.error() != std::errc::timed_out) {
                    logger::info("LaserSimulator: PLC disconnected (Error: {})", readRes.error().message());
                    break;
                }

                // Check if update() transitioned us to Done
                bool done;
                {
                    std::scoped_lock lock(m_mutex);
                    done = m_laserDonePending;
                    if (done)
                        m_laserDonePending = false;
                }

                if (done) {
                    co_await sendStatus("[laser done]");
                }
            }
        }
    }

    auto LaserSimulator::setInternalMode(bool internalMode) -> void
    {
        std::scoped_lock lock(m_mutex);
        m_internalMode = internalMode;
    }

    auto LaserSimulator::isInternalMode() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_internalMode;
    }

    auto LaserSimulator::startLocalMarking(double durationSeconds) -> bool
    {
        std::scoped_lock lock(m_mutex);
        if (!m_internalMode || m_state == State::Laser) {
            return false;
        }

        m_state = State::Laser;
        m_workTimer = std::max(0.1, durationSeconds);
        m_laserDonePending = false;
        return true;
    }

    auto LaserSimulator::acknowledgeDone() -> void
    {
        std::scoped_lock lock(m_mutex);
        if (m_state == State::Done) {
            m_state = State::Idle;
            m_laserDonePending = false;
        }
    }

    auto LaserSimulator::handleCommand(std::string_view cmd) -> coro::Task<void>
    {
        logger::debug("LaserSimulator Command: {}", cmd);

        if (cmd == "[get state]") {
            co_await sendStatus(std::format("[state <{}>]", stateString()));
        }
        else if (cmd == "[open shutter]") {
            if (state() == State::Idle)
                co_await transitionTo(State::Ready);
        }
        else if (cmd == "[close shutter]") {
            if (state() != State::Error)
                co_await transitionTo(State::Idle);
        }
        else if (cmd == "[pilot on]") {
            if (state() == State::Ready)
                co_await transitionTo(State::Pilot);
        }
        else if (cmd == "[pilot off]") {
            if (state() == State::Pilot)
                co_await transitionTo(State::Ready);
        }
        else if (cmd == "[laser on]") {
            if (state() == State::Ready) {
                {
                    std::scoped_lock lock(m_mutex);
                    m_workTimer = 2.0; // 2 seconds of work
                }
                co_await transitionTo(State::Laser);
            }
        }
        else if (cmd == "[ack done]") {
            if (state() == State::Done)
                co_await transitionTo(State::Idle);
        }
        else if (cmd == "[ack error]") {
            if (state() == State::Error)
                co_await transitionTo(State::Idle);
        }

        co_return;
    }

    auto LaserSimulator::sendStatus(std::string_view status) -> coro::Task<void>
    {
        auto* raw = m_link->asRaw();
        if (!raw)
            co_return;

        logger::debug("LaserSimulator TX: {}", status);
        (void)co_await raw->sendFrom("", std::as_bytes(std::span{ status.data(), status.size() }));
    }

    auto LaserSimulator::transitionTo(State newState) -> coro::Task<void>
    {
        {
            std::scoped_lock lock(m_mutex);
            m_state = newState;
        }

        // Auto-responses based on state
        switch (newState) {
            case State::Ready:
                co_await sendStatus("[ready]");
                break;
            case State::Pilot:
                co_await sendStatus("[pilot busy]");
                break;
            case State::Laser:
                co_await sendStatus("[laser busy]");
                break;
            case State::Idle:
                co_await sendStatus("[state <idle>]");
                break;
            default:
                break;
        }
        co_return;
    }
}