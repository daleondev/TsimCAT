#include "LaserController.h"
#include "Simulators/LaserSimulator.hpp"
#include <QCoroTimer>

using namespace std::chrono_literals;

namespace backend::controllers
{
    LaserController::LaserController(std::shared_ptr<core::sim::LaserSimulator> simulator, QObject* parent)
      : QObject(parent)
      , m_simulator(std::move(simulator))
    {
        // Polling loop to sync simulator state to UI properties and drive physics
        [] (LaserController* self) -> QCoro::Task<void> {
            auto lastTime = std::chrono::steady_clock::now();
            while (true) {
                auto now = std::chrono::steady_clock::now();
                double dt = std::chrono::duration<double>(now - lastTime).count();
                lastTime = now;

                if (self->m_simulator) {
                    self->m_simulator->update(dt);
                }

                emit self->tcpStatusChanged();
                emit self->lastMessageChanged();
                co_await QCoro::sleepFor(50ms);
            }
        }(this);
    }

    LaserController::~LaserController() = default;

    QString LaserController::tcpStatus() const
    {
        return m_simulator ? QString::fromStdString(m_simulator->tcpStatus()) : "No Simulator";
    }

    QString LaserController::lastMessage() const
    {
        return m_simulator ? QString::fromStdString(m_simulator->lastMessage()) : "No Simulator";
    }

    void LaserController::startTcpServer()
    {
        if (m_simulator) {
            m_simulator->start();
            // Fire and forget the simulator task
            [] (std::shared_ptr<core::sim::LaserSimulator> sim) -> QCoro::Task<void> {
                co_await sim->initialize();
                co_await sim->run();
            }(m_simulator);
        }
    }
}