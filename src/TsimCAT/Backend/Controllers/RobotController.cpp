#include "RobotController.h"
#include "Simulators/RobotSimulator.hpp"
#include <QTimer>

namespace backend::controllers
{
    RobotController::RobotController(std::shared_ptr<core::sim::RobotSimulator> simulator, QObject* parent)
      : QObject(parent)
      , m_simulator(std::move(simulator))
    {
        auto* timer = new QTimer(this);
        timer->setInterval(100);
        connect(timer, &QTimer::timeout, this, [this]() {
            emit stateChanged();
            emit adsStatusChanged();
        });
        timer->start();
    }

    // --- Status Getters ---
    uint16_t RobotController::jobIdFeedback() const
    {
        return m_simulator ? m_simulator->status().nJobIdFeedback : 0;
    }
    uint8_t RobotController::partTypeMirrored() const
    {
        return m_simulator ? m_simulator->status().nPartTypeMirrored : 0;
    }
    bool RobotController::inMotion() const { return m_simulator ? m_simulator->status().bInMotion : false; }
    bool RobotController::inHome() const { return m_simulator ? m_simulator->status().bInHome : false; }
    bool RobotController::enabled() const { return m_simulator ? m_simulator->status().bEnabled : false; }
    bool RobotController::error() const { return m_simulator ? m_simulator->status().bError : false; }
    bool RobotController::brakeTestOk() const
    {
        return m_simulator ? m_simulator->status().bBrakeTestOk : false;
    }
    bool RobotController::masteringOk() const
    {
        return m_simulator ? m_simulator->status().bMasteringOk : false;
    }
    bool RobotController::inT1() const { return m_simulator ? m_simulator->status().bInT1 : false; }
    bool RobotController::inT2() const { return m_simulator ? m_simulator->status().bInT2 : false; }
    bool RobotController::inAut() const { return m_simulator ? m_simulator->status().bInAut : false; }
    bool RobotController::inExt() const { return m_simulator ? m_simulator->status().bInExt : false; }
    uint8_t RobotController::areaFreeRobot() const
    {
        return m_simulator ? m_simulator->status().nAreaFree_Robot : 0;
    }
    uint32_t RobotController::errorCode() const { return m_simulator ? m_simulator->status().nErrorCode : 0; }

    double RobotController::axis1() const { return m_simulator ? m_simulator->jointAngles()[0] : 0.0; }
    double RobotController::axis2() const { return m_simulator ? m_simulator->jointAngles()[1] : 0.0; }
    double RobotController::axis3() const { return m_simulator ? m_simulator->jointAngles()[2] : 0.0; }
    double RobotController::axis4() const { return m_simulator ? m_simulator->jointAngles()[3] : 0.0; }
    double RobotController::axis5() const { return m_simulator ? m_simulator->jointAngles()[4] : 0.0; }
    double RobotController::axis6() const { return m_simulator ? m_simulator->jointAngles()[5] : 0.0; }

    double RobotController::tcpX() const { return m_simulator ? m_simulator->currentPose().x : 0.0; }
    double RobotController::tcpY() const { return m_simulator ? m_simulator->currentPose().y : 0.0; }
    double RobotController::tcpZ() const { return m_simulator ? m_simulator->currentPose().z : 0.0; }
    double RobotController::tcpRoll() const
    {
        return m_simulator ? m_simulator->currentPose().roll * 180.0 / M_PI : 0.0;
    }
    double RobotController::tcpPitch() const
    {
        return m_simulator ? m_simulator->currentPose().pitch * 180.0 / M_PI : 0.0;
    }
    double RobotController::tcpYaw() const
    {
        return m_simulator ? m_simulator->currentPose().yaw * 180.0 / M_PI : 0.0;
    }

    bool RobotController::gripperGripped() const
    {
        return m_simulator ? m_simulator->isGripperGripped() : false;
    }

    void RobotController::setGripperGripped(bool gripped)
    {
        if (m_simulator) {
            m_simulator->setGripper(gripped);
            emit stateChanged();
        }
    }

    // --- Control Getters ---
    uint16_t RobotController::controlJobId() const { return m_simulator ? m_simulator->control().nJobId : 0; }
    uint8_t RobotController::controlPartType() const
    {
        return m_simulator ? m_simulator->control().nPartType : 0;
    }
    bool RobotController::controlMoveEnable() const
    {
        return m_simulator ? m_simulator->control().bMoveEnable : false;
    }
    bool RobotController::controlReset() const { return m_simulator ? m_simulator->control().bReset : false; }
    uint8_t RobotController::areaFreePlc() const
    {
        return m_simulator ? m_simulator->control().nAreaFree_PLC : 0;
    }

    QString RobotController::adsStatus() const
    {
        return m_simulator ? QString::fromStdString(m_simulator->adsStatus()) : "Offline";
    }

    void RobotController::connectAds()
    {
        if (m_simulator) {
            m_simulator->start();
            emit adsStatusChanged();
        }
    }

    void RobotController::setJoints(double j1, double j2, double j3, double j4, double j5, double j6)
    {
        if (m_simulator) {
            double angles[6] = { j1, j2, j3, j4, j5, j6 };
            m_simulator->setJointAngles(angles);
            emit stateChanged();
        }
    }

    bool RobotController::setTcp(double x, double y, double z, double r, double p, double w)
    {
        if (m_simulator) {
            core::sim::Pose pose{ x, y, z, r * M_PI / 180.0, p * M_PI / 180.0, w * M_PI / 180.0 };
            bool success = m_simulator->setTargetPose(pose);
            if (success) {
                emit stateChanged();
            }
            return success;
        }
        return false;
    }

    void RobotController::triggerJob(int jobId)
    {
        if (m_simulator) {
            m_simulator->triggerJob(static_cast<uint16_t>(jobId));
            emit stateChanged();
        }
    }
}