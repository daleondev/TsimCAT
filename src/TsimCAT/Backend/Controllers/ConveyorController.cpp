#include "ConveyorController.h"
#include "Simulators/ConveyorSimulator.hpp"
#include <QVariantMap>

namespace backend::controllers
{
    ConveyorController::ConveyorController(std::shared_ptr<core::sim::ConveyorSimulator> simulator,
                                           QObject* parent)
      : QObject(parent)
      , m_simulator(std::move(simulator))
    {
    }

    QString ConveyorController::name() const { return QString::fromStdString(m_simulator->name()); }

    double ConveyorController::length() const { return m_simulator->length(); }

    double ConveyorController::speed() const { return m_simulator->speed(); }

    void ConveyorController::setSpeed(double speed)
    {
        if (m_simulator->speed() != speed) {
            m_simulator->setSpeed(speed);
            emit speedChanged();
        }
    }

    bool ConveyorController::isRunning() const { return m_simulator->isRunning(); }

    void ConveyorController::setRunning(bool running)
    {
        if (m_simulator->isRunning() != running) {
            m_simulator->setRunning(running);
            emit stateChanged();
        }
    }

    bool ConveyorController::autoSpawn() const { return m_simulator->autoSpawn(); }

    void ConveyorController::setAutoSpawn(bool autoSpawn)
    {
        if (m_simulator->autoSpawn() != autoSpawn) {
            m_simulator->setAutoSpawn(autoSpawn);
            emit stateChanged();
        }
    }

    bool ConveyorController::autoLogic() const { return m_simulator->autoLogic(); }

    void ConveyorController::setAutoLogic(bool enable)
    {
        if (m_simulator->autoLogic() != enable) {
            m_simulator->setAutoLogic(enable);
            emit stateChanged();
        }
    }

    bool ConveyorController::consumeAtEndSensor() const { return m_simulator->consumeAtEndSensor(); }

    void ConveyorController::setConsumeAtEndSensor(bool enable)
    {
        if (m_simulator->consumeAtEndSensor() != enable) {
            m_simulator->setConsumeAtEndSensor(enable);
            emit stateChanged();
        }
    }

    bool ConveyorController::partAtEndSensor() const { return m_simulator->peekPartAtEnd().has_value(); }

    bool ConveyorController::damperOpen() const { return m_simulator->damperOpen(); }

    void ConveyorController::setDamperOpen(bool open)
    {
        if (m_simulator->damperOpen() != open) {
            m_simulator->setDamperOpen(open);
            emit stateChanged();
        }
    }

    QVariantList ConveyorController::parts() const
    {
        QVariantList list;
        auto parts = m_simulator->parts();
        for (const auto& part : parts) {
            QVariantMap map;
            map["id"] = part.id;
            map["type"] = part.type;
            map["position"] = part.position;
            map["width"] = part.width;
            map["length"] = part.length;
            map["height"] = part.height;
            list.append(map);
        }
        return list;
    }

    QVariantList ConveyorController::sensors() const
    {
        QVariantList list;
        auto sensors = m_simulator->sensors();
        for (bool state : sensors) {
            list.append(state);
        }
        return list;
    }

    void ConveyorController::spawnPart(int type) { m_simulator->spawnPart(static_cast<uint8_t>(type)); }

    bool ConveyorController::despawnPart() { return m_simulator->takePartAtEnd().has_value(); }

    void ConveyorController::clearParts() { m_simulator->clearParts(); }
}
