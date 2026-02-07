#include "Kinematics.hpp"
#include <chain.hpp>
#include <chainfksolverpos_recursive.hpp>
#include <chainiksolverpos_lma.hpp>
#include <frames_io.hpp>

namespace core::sim
{
    struct Kinematics::Impl
    {
        KDL::Chain chain;
        std::unique_ptr<KDL::ChainFkSolverPos_recursive> fkSolver;
        std::unique_ptr<KDL::ChainIkSolverPos_LMA> ikSolver;

        Impl()
        {
            using namespace KDL;
            
            // Replicating the RobotModel.qml structure
            // 1. Base to Joint 1
            chain.addSegment(Segment(Joint(Joint::None), Frame(Vector(0, 0, 0.450))));
            
            // 2. Joint 1 (RotZ) to Joint 2
            chain.addSegment(Segment(Joint(Joint::RotZ), Frame(Vector(0.150, 0, 0))));
            
            // 3. Joint 2 (RotY) to Joint 3
            chain.addSegment(Segment(Joint(Joint::RotY), Frame(Vector(0.610, 0, 0))));
            
            // 4. Joint 3 (RotY) to Joint 4
            chain.addSegment(Segment(Joint(Joint::RotY), Frame(Vector(0, 0, 0.02))));
            
            // 5. Joint 4 (RotX) to Joint 5
            chain.addSegment(Segment(Joint(Joint::RotX), Frame(Vector(0.660, 0, 0))));
            
            // 6. Joint 5 (RotY) to Joint 6
            chain.addSegment(Segment(Joint(Joint::RotY), Frame(Vector(0.080, 0, 0))));
            
            // 7. Joint 6 (RotX) to Flange
            chain.addSegment(Segment(Joint(Joint::RotX), Frame::Identity()));

            fkSolver = std::make_unique<ChainFkSolverPos_recursive>(chain);
            
            // LMA solver is more robust for 6DOF than simple NR
            ikSolver = std::make_unique<ChainIkSolverPos_LMA>(chain);
        }
    };

    Kinematics::Kinematics() : m_impl(std::make_unique<Impl>()) {}
    Kinematics::~Kinematics() = default;

    auto Kinematics::forward(const std::array<double, 6>& jointAngles) const -> Pose
    {
        KDL::JntArray jntPos(6);
        for (int i = 0; i < 6; ++i) {
            jntPos(i) = jointAngles[i];
        }

        KDL::Frame cartPos;
        m_impl->fkSolver->JntToCart(jntPos, cartPos);

        Pose pose;
        pose.x = cartPos.p.x();
        pose.y = cartPos.p.y();
        pose.z = cartPos.p.z();
        cartPos.M.GetRPY(pose.roll, pose.pitch, pose.yaw);
        return pose;
    }

    auto Kinematics::inverse(const Pose& target, const std::array<double, 6>& seed) const -> std::vector<double>
    {
        KDL::JntArray jntSeed(6);
        for (int i = 0; i < 6; ++i) {
            jntSeed(i) = seed[i];
        }

        KDL::Frame cartGoal(KDL::Rotation::RPY(target.roll, target.pitch, target.yaw), 
                            KDL::Vector(target.x, target.y, target.z));

        KDL::JntArray jntResult(6);
        int ret = m_impl->ikSolver->CartToJnt(jntSeed, cartGoal, jntResult);

        if (ret >= 0) {
            std::vector<double> result(6);
            for (int i = 0; i < 6; ++i) {
                result[i] = jntResult(i);
            }
            return result;
        }

        return {};
    }
}
