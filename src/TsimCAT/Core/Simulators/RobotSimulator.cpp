#include "RobotSimulator.hpp"
#include "Logger/Logger.hpp"
#include "Link/Symbolic/ISymbolicLink.hpp"
#include "Coroutines/Task.hpp"
#include <cmath>

namespace core::sim
{
    RobotSimulator::RobotSimulator(std::shared_ptr<link::ILink> link)
        : m_link(std::move(link))
    {
        // Initial hardware-like state
        m_status.bInHome = 1;
        m_status.bEnabled = 1;
        m_status.bInAut = 1;
        m_status.bMasteringOk = 1;
        m_status.bBrakeTestOk = 1;

        // Initialize to a nice "Home" position (Degrees)
        m_jointAngles[0] = 0.0;    // Axis 1
        m_jointAngles[1] = -90.0;  // Axis 2 (Upright)
        m_jointAngles[2] = 90.0;   // Axis 3 (Horizontal)
        m_jointAngles[3] = 0.0;    // Axis 4
        m_jointAngles[4] = 0.0;    // Axis 5
        m_jointAngles[5] = 0.0;    // Axis 6

        // Test Kinematics
        std::array<double, 6> rads;
        for (int i = 0; i < 6; ++i) rads[i] = m_jointAngles[i] * M_PI / 180.0;
        auto pose = m_kinematics.forward(rads);
        logger::info("RobotSimulator: Initial Pose [X: {:.3f}, Y: {:.3f}, Z: {:.3f}]", pose.x, pose.y, pose.z);
    }

    RobotSimulator::~RobotSimulator()
    {
        stop();
    }

    auto RobotSimulator::initialize() -> coro::Task<result::Result<void>>
    {
        if (auto* client = m_link->asClient()) {
            logger::info("RobotSimulator: Connecting to ADS...");
            auto res = co_await client->connect();
            if (!res) {
                logger::error("RobotSimulator: ADS Connection failed: {}", res.error().message());
                co_return std::unexpected(res.error());
            } else {
                logger::info("RobotSimulator: ADS Connected");
            }
        }
        co_return result::success();
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
        // Logic Simulation
        std::scoped_lock lock(m_mutex);
        
        m_status.nPartTypeMirrored = m_control.nPartType;
        
        if (m_control.bMoveEnable && !m_status.bError) {
            m_status.bInMotion = 1;
            m_status.bInHome = 0;
        } else {
            m_status.bInMotion = 0;
            if (m_control.nJobId == 0) m_status.bInHome = 1;
        }

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

    auto RobotSimulator::jointAngles() const -> const double*
    {
        std::scoped_lock lock(m_mutex);
        return m_jointAngles;
    }

    auto RobotSimulator::run() -> coro::Task<void>
    {
        auto* symbolic = m_link->asSymbolic();
        if (!symbolic) co_return;

        while (m_running) {
            // Only communicate if actually connected to avoid floods
            if (m_link->status() == link::Status::Connected) {
                // 1. Read Commands
                auto ctrlRes = co_await symbolic->read<RobotControl>("MAIN.stRobotControl");
                if (ctrlRes) {
                    std::scoped_lock lock(m_mutex);
                    m_control = *ctrlRes;
                }

                // 2. Write Status
                RobotStatus s;
                {
                    std::scoped_lock lock(m_mutex);
                    s = m_status;
                }
                (void)co_await symbolic->write("MAIN.stRobotStatus", s);
            } else {
                // Throttled wait when disconnected
                co_await coro::sleep(std::chrono::milliseconds(500));
            }
            
                        // Standard loop throttle
            
                        co_await coro::sleep(std::chrono::milliseconds(50));
            
                    }
            
                }
            
            
            
                auto RobotSimulator::currentPose() const -> Pose
            
                {
            
                    std::scoped_lock lock(m_mutex);
            
                    std::array<double, 6> rads;
            
                    for (int i = 0; i < 6; ++i) rads[i] = m_jointAngles[i] * M_PI / 180.0;
            
                    return m_kinematics.forward(rads);
            
                }
            
            }
            
            