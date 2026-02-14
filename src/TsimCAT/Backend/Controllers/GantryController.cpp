#include "GantryController.h"

#include "Simulators/GantrySimulator.hpp"

namespace backend::controllers
{
    GantryController::GantryController(std::shared_ptr<core::sim::GantrySimulator> simulator, QObject* parent)
      : QObject(parent)
      , m_simulator(std::move(simulator))
    {
    }

    double GantryController::xPos() const { return m_simulator ? m_simulator->xPos() : 0.0; }

    double GantryController::zPos() const { return m_simulator ? m_simulator->zPos() : 0.0; }

    bool GantryController::gripperGripped() const
    {
        return m_simulator ? m_simulator->gripperGripped() : false;
    }

    bool GantryController::autoTransfer() const { return m_simulator ? m_simulator->autoTransfer() : false; }

    void GantryController::setAutoTransfer(bool enabled)
    {
        if (!m_simulator) {
            return;
        }

        if (m_simulator->autoTransfer() == enabled) {
            return;
        }

        m_simulator->setAutoTransfer(enabled);
        emit stateChanged();
    }

    bool GantryController::hasCarriedPart() const
    {
        return m_simulator ? m_simulator->hasCarriedPart() : false;
    }

    int GantryController::carriedPartType() const
    {
        return m_simulator ? static_cast<int>(m_simulator->carriedPartType()) : 0;
    }

    QString GantryController::status() const
    {
        return m_simulator ? QString::fromStdString(m_simulator->statusText()) : QStringLiteral("Offline");
    }
}
