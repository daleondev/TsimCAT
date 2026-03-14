#include "RotaryTableController.h"

#include "Simulators/RotaryTableSimulator.hpp"

namespace backend::controllers
{
    RotaryTableController::RotaryTableController(std::shared_ptr<core::sim::RotaryTableSimulator> simulator,
                                                 QObject* parent)
      : QObject(parent)
      , m_simulator(std::move(simulator))
    {
    }

    QString RotaryTableController::name() const
    {
        return m_simulator ? QString::fromStdString(m_simulator->name()) : QStringLiteral("RotaryTable");
    }

    double RotaryTableController::angleDegrees() const
    {
        return m_simulator ? m_simulator->currentAngleDegrees() : 0.0;
    }

    bool RotaryTableController::partPresent() const
    {
        return m_simulator ? m_simulator->partPresent() : false;
    }

    bool RotaryTableController::readyToPick() const
    {
        return m_simulator ? m_simulator->readyToPick() : false;
    }

    bool RotaryTableController::atLoadPosition() const
    {
        return m_simulator ? m_simulator->atLoadPosition() : false;
    }

    bool RotaryTableController::busy() const { return m_simulator ? m_simulator->isBusy() : false; }

    void RotaryTableController::queuePart()
    {
        if (m_simulator) {
            m_simulator->queuePart();
            emit stateChanged();
        }
    }
}