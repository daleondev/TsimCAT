#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::sim
{
    class GantrySimulator;
}

namespace backend::controllers
{
    class GantryController : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("Managed by Backend")

        Q_PROPERTY(double xPos READ xPos NOTIFY stateChanged)
        Q_PROPERTY(double zPos READ zPos NOTIFY stateChanged)
        Q_PROPERTY(bool gripperGripped READ gripperGripped NOTIFY stateChanged)
        Q_PROPERTY(bool autoTransfer READ autoTransfer WRITE setAutoTransfer NOTIFY stateChanged)
        Q_PROPERTY(bool hasCarriedPart READ hasCarriedPart NOTIFY stateChanged)
        Q_PROPERTY(int carriedPartType READ carriedPartType NOTIFY stateChanged)
        Q_PROPERTY(QString status READ status NOTIFY stateChanged)

      public:
        explicit GantryController(std::shared_ptr<core::sim::GantrySimulator> simulator,
                                  QObject* parent = nullptr);

        double xPos() const;
        double zPos() const;
        bool gripperGripped() const;
        bool autoTransfer() const;
        void setAutoTransfer(bool enabled);
        bool hasCarriedPart() const;
        int carriedPartType() const;
        QString status() const;

      signals:
        void stateChanged();

      private:
        std::shared_ptr<core::sim::GantrySimulator> m_simulator;
    };
}
