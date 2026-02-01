#include "RobotSimulator.hpp"
#include "Logger/Logger.hpp"
#include "Link/Symbolic/ISymbolicLink.hpp"
#include <thread>

namespace core::sim
{
    RobotSimulator::RobotSimulator(std::shared_ptr<link::ILink> link)
        : m_link(std::move(link))
    {
        // Initial state
        m_status.bInHome = 1;
        m_status.bEnabled = 1;
        m_status.bInAut = 1;
        m_status.bMasteringOk = 1;
        m_status.bBrakeTestOk = 1;
    }

    RobotSimulator::~RobotSimulator()
    {
        stop();
    }

    auto RobotSimulator::initialize() -> coro::Task<void>
    {
        if (auto* client = m_link->asClient()) {
            logger::info("RobotSimulator: Connecting to ADS...");
            auto res = co_await client->connect();
            if (!res) {
                logger::error("RobotSimulator: ADS Connection failed: {}", res.error().message());
            } else {
                logger::info("RobotSimulator: ADS Connected");
            }
        }
        co_return;
    }

    auto RobotSimulator::start() -> void
    {
        m_running = true;
    }

    auto RobotSimulator::stop() -> void
    {
        m_running = false;
    }

    auto RobotSimulator::update(double deltaTimeSeconds) -> void
    {
        // Simple logic simulation
        std::scoped_lock lock(m_mutex);
        
        // Mirror part type as per protocol
        m_status.nPartTypeMirrored = m_control.nPartType;
        
        // Simulate motion if enable is on
        if (m_control.bMoveEnable) {
            m_status.bInMotion = 1;
            m_status.bInHome = 0;
        } else {
            m_status.bInMotion = 0;
            if (m_control.nJobId == 0) m_status.bInHome = 1;
        }

        // Job Feedback
        m_status.nJobIdFeedback = m_control.nJobId;
    }

    auto RobotSimulator::control() const -> RobotControl
    {
        std::scoped_lock lock(m_mutex);
        return m_control;
    }

    auto RobotSimulator::status() const -> RobotStatus
    {
        std::scoped_lock lock(m_mutex);
        return m_status;
    }

    auto RobotSimulator::adsStatus() const -> std::string
    {
        if (!m_link) return "No Link";
        switch (m_link->status()) {
            case link::Status::Disconnected: return "Disconnected";
            case link::Status::Connecting: return "Connecting";
            case link::Status::Connected: return "Connected";
            case link::Status::Faulty: return "Faulty";
            default: return "Unknown";
        }
    }

    auto RobotSimulator::run() -> coro::Task<void>
    {
        auto* symbolic = m_link->asSymbolic();
        if (!symbolic) co_return;

        while (m_running) {
            if (m_link->status() == link::Status::Connected) {
                // Read control struct from PLC
                auto ctrlRes = co_await symbolic->read<RobotControl>("MAIN.stRobotControl");
                if (ctrlRes) {
                    std::scoped_lock lock(m_mutex);
                    m_control = *ctrlRes;
                }

                // Write status struct back to PLC
                RobotStatus currentStatus;
                {
                    std::scoped_lock lock(m_mutex);
                    currentStatus = m_status;
                }
                (void)co_await symbolic->write("MAIN.stRobotStatus", currentStatus);
            }
            
            // Throttle loop
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        co_return;
    }
}
