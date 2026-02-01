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

    uint16_t RobotController::jobId() const { return m_simulator ? m_simulator->status().nJobIdFeedback : 0; }
    uint8_t RobotController::partType() const { return m_simulator ? m_simulator->status().nPartTypeMirrored : 0; }
    bool RobotController::inMotion() const { return m_simulator ? m_simulator->status().bInMotion : false; }
    bool RobotController::inHome() const { return m_simulator ? m_simulator->status().bInHome : false; }
    bool RobotController::enabled() const { return m_simulator ? m_simulator->status().bEnabled : false; }
    
    QString RobotController::adsStatus() const { 
        return m_simulator ? QString::fromStdString(m_simulator->adsStatus()) : "Offline"; 
    }

    void RobotController::connectAds()
    {
        if (m_simulator) {
            m_simulator->start();
            [] (std::shared_ptr<core::sim::RobotSimulator> sim) -> QCoro::Task<void> {
                co_await sim->initialize();
                co_await sim->run();
            }(m_simulator);
        }
    }
}
