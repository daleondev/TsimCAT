#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::sim
{
    class ConveyorSimulator;
}

namespace backend::controllers
{
    class ConveyorController : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("Managed by Backend")

        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(double length READ length CONSTANT)
        Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
        Q_PROPERTY(bool isRunning READ isRunning WRITE setRunning NOTIFY stateChanged)
        Q_PROPERTY(bool autoSpawn READ autoSpawn WRITE setAutoSpawn NOTIFY stateChanged)
        Q_PROPERTY(bool autoLogic READ autoLogic WRITE setAutoLogic NOTIFY stateChanged)
        Q_PROPERTY(
          bool consumeAtEndSensor READ consumeAtEndSensor WRITE setConsumeAtEndSensor NOTIFY stateChanged)
        Q_PROPERTY(bool partAtEndSensor READ partAtEndSensor NOTIFY stateChanged)
        Q_PROPERTY(bool damperOpen READ damperOpen WRITE setDamperOpen NOTIFY stateChanged)
        Q_PROPERTY(QVariantList parts READ parts NOTIFY stateChanged)
        Q_PROPERTY(QVariantList sensors READ sensors NOTIFY stateChanged)

      public:
        explicit ConveyorController(std::shared_ptr<core::sim::ConveyorSimulator> simulator,
                                    QObject* parent = nullptr);

        QString name() const;
        double length() const;
        double speed() const;
        void setSpeed(double speed);

        bool isRunning() const;
        void setRunning(bool running);

        bool autoSpawn() const;
        void setAutoSpawn(bool autoSpawn);

        bool autoLogic() const;
        void setAutoLogic(bool enable);

        bool consumeAtEndSensor() const;
        void setConsumeAtEndSensor(bool enable);
        bool partAtEndSensor() const;

        bool damperOpen() const;
        void setDamperOpen(bool open);

        QVariantList parts() const;
        QVariantList sensors() const;

        Q_INVOKABLE void spawnPart(int type);
        Q_INVOKABLE bool despawnPart();
        Q_INVOKABLE void clearParts();

      signals:
        void speedChanged();
        void stateChanged();

      private:
        std::shared_ptr<core::sim::ConveyorSimulator> m_simulator;
    };
}
