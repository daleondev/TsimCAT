#include "RobotSimulator.hpp"
#include "Coroutines/Task.hpp"
#include "Link/Symbolic/ISymbolicLink.hpp"
#include "Logger/Logger.hpp"
#include <cmath>
#include <numbers>

#include <ompl/base/SpaceInformation.h>
#include <ompl/base/spaces/RealVectorStateSpace.h>
#include <ompl/geometric/PathSimplifier.h>
#include <ompl/geometric/SimpleSetup.h>
#include <ompl/geometric/planners/rrt/RRTConnect.h>

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
        m_jointAngles[0] = 0.0;   // Axis 1
        m_jointAngles[1] = -90.0; // Axis 2 (Upright)
        m_jointAngles[2] = 90.0;  // Axis 3 (Horizontal)
        m_jointAngles[3] = 0.0;   // Axis 4
        m_jointAngles[4] = 0.0;   // Axis 5
        m_jointAngles[5] = 0.0;   // Axis 6

        for (int i = 0; i < 6; ++i)
            m_targetJointAngles[i] = m_jointAngles[i];

        // Test Kinematics
        std::array<double, 6> rads;
        for (int i = 0; i < 6; ++i)
            rads[i] = m_jointAngles[i] * std::numbers::pi / 180.0;
        auto pose = m_kinematics.forward(rads);
        logger::info(
          "RobotSimulator: Initial Pose [X: {:.3f}, Y: {:.3f}, Z: {:.3f}]", pose.x, pose.y, pose.z);
    }

    RobotSimulator::~RobotSimulator() { stop(); }

    auto RobotSimulator::initialize() -> coro::Task<result::Result<void>>
    {
        if (auto* client = m_link->asClient()) {
            logger::info("RobotSimulator: Connecting to ADS...");
            auto res = co_await client->connect();
            if (!res) {
                logger::error("RobotSimulator: ADS Connection failed: {}", res.error().message());
                co_return std::unexpected(res.error());
            }
            else {
                logger::info("RobotSimulator: ADS Connected");
            }
        }
        co_return result::success();
    }

    auto RobotSimulator::start() -> void { m_running = true; }

    auto RobotSimulator::stop() -> void { m_running = false; }

    auto RobotSimulator::update(double deltaTimeSeconds) -> void
    {
        // Logic Simulation
        std::scoped_lock lock(m_mutex);

        m_status.nPartTypeMirrored = m_control.nPartType;

        if (m_control.bMoveEnable && !m_status.bError) {
            // 1. Check if Job ID changed -> Plan New Trajectory
            if (m_control.nJobId != m_lastTargetJobId) {
                m_lastTargetJobId = m_control.nJobId;

                using std::numbers::pi;
                Pose targetPose;
                bool validJob = true;

                switch (m_control.nJobId) {
                    case 1:
                        targetPose = { 905.0, 0.0, 1080.0, 0.0, 0.0, 0.0 };
                        break;
                    case 2:
                        targetPose = { 615.0, 650.0, 520.0, 0.0, 0.0, 90.0 * pi / 180.0 };
                        break;
                    case 3:
                    case 4:
                        targetPose = { 1270.0, 550.0, 580.0, 0.0, 0.0, 45.0 * pi / 180.0 };
                        break;
                    case 5:
                    case 6:
                        targetPose = { 1270.0, -550.0, 580.0, 0.0, 0.0, -45.0 * pi / 180.0 };
                        break;
                    case 7:
                        targetPose = { 615.0, -690.0, 520.0, 0.0, 0.0, -90.0 * pi / 180.0 };
                        break;
                    default:
                        validJob = false;
                        break;
                }

                if (validJob) {

                    std::array<double, 6> startJoints;

                    for (int i = 0; i < 6; ++i)
                        startJoints[i] = m_jointAngles[i];

                    std::array<double, 6> homeJoints = { 0.0, -90.0, 90.0, 0.0, 0.0, 0.0 };

                    // Get target joints from IK - Use homeJoints as seed since we plan from there

                    std::array<double, 6> homeRads;

                    for (int i = 0; i < 6; ++i)
                        homeRads[i] = homeJoints[i] * pi / 180.0;

                    auto targetRadsVec = m_kinematics.inverse(targetPose, homeRads);

                    if (!targetRadsVec.empty()) {

                        std::array<double, 6> targetJoints;
                        for (int i = 0; i < 6; ++i)
                            targetJoints[i] = targetRadsVec[i] * 180.0 / pi;

                        // PLAN: Start -> Home
                        auto path1 = planTrajectory(startJoints, homeJoints);
                        // PLAN: Home -> Target
                        auto path2 = planTrajectory(homeJoints, targetJoints);

                        // Combine paths with blending
                        m_currentTrajectory.clear();
                        m_currentTrajectory.insert(m_currentTrajectory.end(), path1.begin(), path1.end());

                        if (!m_currentTrajectory.empty() && !path2.empty()) {
                            // Remove overlapping Home waypoint to allow smoother blending in next step if we
                            // had OMPL path types here. Since we already did interpolation in planTrajectory,
                            // we just join them.
                            m_currentTrajectory.pop_back();
                        }
                        m_currentTrajectory.insert(m_currentTrajectory.end(), path2.begin(), path2.end());
                        m_trajectoryStep = 0;

                        logger::info(
                          "RobotSimulator: Planned blended trajectory for Job {} with {} waypoints",
                          m_control.nJobId,
                          m_currentTrajectory.size());
                    }
                    else {
                        logger::error("RobotSimulator: IK failed for Job {}", m_control.nJobId);
                    }
                }
            }

            // 2. Follow Trajectory
            if (!m_currentTrajectory.empty() && m_trajectoryStep < m_currentTrajectory.size()) {
                const auto& target = m_currentTrajectory[m_trajectoryStep];

                // Interpolate towards the current waypoint
                const double jointSpeedDegreesPerSecond = 150.0;
                const double step = jointSpeedDegreesPerSecond * deltaTimeSeconds;
                bool waypointReached = true;

                for (int i = 0; i < 6; ++i) {
                    double diff = target[i] - m_jointAngles[i];
                    if (std::abs(diff) > step) {
                        m_jointAngles[i] += std::copysign(step, diff);
                        waypointReached = false;
                    }
                    else {
                        m_jointAngles[i] = target[i];
                    }
                }

                if (waypointReached) {
                    m_trajectoryStep++;
                }

                m_status.bInMotion = 1;
                m_status.bInHome = 0;
            }
            else {
                m_status.bInMotion = 0;
                // Check if we are at home position waypoints
                std::array<double, 6> homeJoints = { 0.0, -90.0, 90.0, 0.0, 0.0, 0.0 };
                bool atHome = true;
                for (int i = 0; i < 6; ++i) {
                    if (std::abs(m_jointAngles[i] - homeJoints[i]) > 0.1) {
                        atHome = false;
                        break;
                    }
                }
                m_status.bInHome = atHome ? 1 : 0;
            }
        }
        else {
            m_status.bInMotion = 0;
        }

        m_status.nJobIdFeedback = m_control.nJobId;
    }

    auto RobotSimulator::planTrajectory(const std::array<double, 6>& startJoints,
                                        const std::array<double, 6>& targetJoints)
      -> std::vector<std::array<double, 6>>
    {
        namespace ob = ompl::base;
        namespace og = ompl::geometric;

        // 1. Create State Space
        auto space(std::make_shared<ob::RealVectorStateSpace>(6));

        // 2. Set Bounds
        ob::RealVectorBounds bounds(6);
        bounds.setLow(0, -180.0);
        bounds.setHigh(0, 180.0); // A1
        bounds.setLow(1, -150.0);
        bounds.setHigh(1, 150.0); // A2
        bounds.setLow(2, -150.0);
        bounds.setHigh(2, 150.0); // A3
        bounds.setLow(3, -180.0);
        bounds.setHigh(3, 180.0); // A4
        bounds.setLow(4, -125.0);
        bounds.setHigh(4, 125.0); // A5
        bounds.setLow(5, -180.0);
        bounds.setHigh(5, 180.0); // A6
        space->setBounds(bounds);

        // 3. Simple Setup
        og::SimpleSetup ss(space);

        // State validity checker
        ss.setStateValidityChecker([](const ob::State*) { return true; });

        // 4. Start and Goal States
        ob::ScopedState<> start(space);
        for (int i = 0; i < 6; ++i)
            start[i] = startJoints[i];

        ob::ScopedState<> goal(space);
        for (int i = 0; i < 6; ++i)
            goal[i] = targetJoints[i];

        ss.setStartAndGoalStates(start, goal);

        // 5. Solve
        ob::PlannerStatus solved = ss.solve(0.1);

        if (solved) {
            ss.simplifySolution();
            auto path = ss.getSolutionPath();

            // Apply B-Spline smoothing for "nice blending"
            ss.getPathSimplifier()->smoothBSpline(path);
            path.interpolate(20);

            std::vector<std::array<double, 6>> result;
            for (size_t i = 0; i < path.getStateCount(); ++i) {
                const auto* state = path.getState(i)->as<ob::RealVectorStateSpace::StateType>();
                std::array<double, 6> waypoint;
                for (int j = 0; j < 6; ++j)
                    waypoint[j] = (*state)[j];
                result.push_back(waypoint);
            }
            return result;
        }

        // Fallback: Linear interpolation if planning fails
        return { startJoints, targetJoints };
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
        if (!m_link)
            return "No Link";
        switch (m_link->status()) {
            case link::Status::Disconnected:
                return "Disconnected";
            case link::Status::Connecting:
                return "Connecting";
            case link::Status::Connected:
                return "Connected";
            case link::Status::Faulty:
                return "Faulty";
            default:
                return "Unknown";
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
        if (!symbolic)
            co_return;

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
            }
            else {
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
        for (int i = 0; i < 6; ++i)
            rads[i] = m_jointAngles[i] * std::numbers::pi / 180.0;
        return m_kinematics.forward(rads);
    }

    auto RobotSimulator::isGripperGripped() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_gripperGripped;
    }

    auto RobotSimulator::setGripper(bool gripped) -> void
    {
        std::scoped_lock lock(m_mutex);
        m_gripperGripped = gripped;
    }

    auto RobotSimulator::setJointAngles(const double* anglesDegrees) -> void
    {
        std::scoped_lock lock(m_mutex);
        for (int i = 0; i < 6; ++i) {
            m_jointAngles[i] = anglesDegrees[i];
        }
        m_currentTrajectory.clear();
    }

    auto RobotSimulator::setTargetPose(const Pose& pose) -> bool
    {
        std::array<double, 6> seed;
        {
            std::scoped_lock lock(m_mutex);
            for (int i = 0; i < 6; ++i)
                seed[i] = m_jointAngles[i] * std::numbers::pi / 180.0;
        }

        auto joints = m_kinematics.inverse(pose, seed);
        if (joints.empty()) {
            return false;
        }

        std::array<double, 6> start;
        std::array<double, 6> target;
        {
            std::scoped_lock lock(m_mutex);
            for (int i = 0; i < 6; ++i) {
                start[i] = m_jointAngles[i];
                target[i] = joints[i] * 180.0 / std::numbers::pi;
            }
        }

        auto path = planTrajectory(start, target);
        std::scoped_lock lock(m_mutex);
        m_currentTrajectory = std::move(path);
        m_trajectoryStep = 0;
        return true;
    }

    auto RobotSimulator::triggerJob(uint16_t jobId) -> void
    {
        std::scoped_lock lock(m_mutex);
        m_control.nJobId = jobId;
        m_control.bMoveEnable = 1; // Auto-enable move for convenience in simulation
    }
}