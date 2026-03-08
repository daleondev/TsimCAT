#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::sim
{
    class RotaryTableSimulator;
}

namespace backend::controllers
{
    class RotaryTableController : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("Managed by Backend")

        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(double angleDegrees READ angleDegrees NOTIFY stateChanged)
        Q_PROPERTY(bool partPresent READ partPresent NOTIFY stateChanged)
        Q_PROPERTY(bool readyToPick READ readyToPick NOTIFY stateChanged)
        Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
        Q_PROPERTY(int partType READ partType NOTIFY stateChanged)

      public:
        explicit RotaryTableController(std::shared_ptr<core::sim::RotaryTableSimulator> simulator,
                                       QObject* parent = nullptr);

        QString name() const;
        double angleDegrees() const;
        bool partPresent() const;
        bool readyToPick() const;
        bool busy() const;
        int partType() const;

        Q_INVOKABLE void queuePart(int partType);

      signals:
        void stateChanged();

      private:
        std::shared_ptr<core::sim::RotaryTableSimulator> m_simulator;
    };
}