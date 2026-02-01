#include "RobotController.h"
#include "Simulators/RobotSimulator.hpp"
#include <QCoroTimer>

using namespace std::chrono_literals;

namespace backend::controllers
{
    RobotController::RobotController(std::shared_ptr<core::sim::RobotSimulator> simulator, QObject* parent)
      : QObject(parent)
      , m_simulator(std::move(simulator))
    {
        // Sync loop to UI
        [] (RobotController* self) -> QCoro::Task<void> {
            while (true) {
                emit self->stateChanged();
                emit self->adsStatusChanged();
                co_await QCoro::sleepFor(100ms);
            }
        }(this);
    }

    // --- Status Getters ---
    uint16_t RobotController::jobIdFeedback() const { return m_simulator ? m_simulator->status().nJobIdFeedback : 0; }
    uint8_t RobotController::partTypeMirrored() const { return m_simulator ? m_simulator->status().nPartTypeMirrored : 0; }
    bool RobotController::inMotion() const { return m_simulator ? m_simulator->status().bInMotion : false; }
    bool RobotController::inHome() const { return m_simulator ? m_simulator->status().bInHome : false; }
    bool RobotController::enabled() const { return m_simulator ? m_simulator->status().bEnabled : false; }
    bool RobotController::error() const { return m_simulator ? m_simulator->status().bError : false; }
    bool RobotController::brakeTestOk() const { return m_simulator ? m_simulator->status().bBrakeTestOk : false; }
    bool RobotController::masteringOk() const { return m_simulator ? m_simulator->status().bMasteringOk : false; }
    bool RobotController::inT1() const { return m_simulator ? m_simulator->status().bInT1 : false; }
    bool RobotController::inT2() const { return m_simulator ? m_simulator->status().bInT2 : false; }
    bool RobotController::inAut() const { return m_simulator ? m_simulator->status().bInAut : false; }
    bool RobotController::inExt() const { return m_simulator ? m_simulator->status().bInExt : false; }
    uint8_t RobotController::areaFreeRobot() const { return m_simulator ? m_simulator->status().nAreaFree_Robot : 0; }
    uint32_t RobotController::errorCode() const { return m_simulator ? m_simulator->status().nErrorCode : 0; }

    // --- Control Getters ---
    uint16_t RobotController::controlJobId() const { return m_simulator ? m_simulator->control().nJobId : 0; }
    uint8_t RobotController::controlPartType() const { return m_simulator ? m_simulator->control().nPartType : 0; }
    bool RobotController::controlMoveEnable() const { return m_simulator ? m_simulator->control().bMoveEnable : false; }
    bool RobotController::controlReset() const { return m_simulator ? m_simulator->control().bReset : false; }
    uint8_t RobotController::areaFreePlc() const { return m_simulator ? m_simulator->control().nAreaFree_PLC : 0; }
    
    QString RobotController::adsStatus() const { 
        return m_simulator ? QString::fromStdString(m_simulator->adsStatus()) : "Offline"; 
    }

    void RobotController::connectAds()
    {
        if (m_simulator) {
            if (m_simulator->adsStatus() != "Disconnected" && m_simulator->adsStatus() != "Faulty") return;

            m_simulator->start();
            [] (std::shared_ptr<core::sim::RobotSimulator> sim) -> QCoro::Task<void> {
                auto res = co_await sim->initialize();
                if (res) {
                    co_await sim->run();
                }
            }(m_simulator);
        }
    }
}