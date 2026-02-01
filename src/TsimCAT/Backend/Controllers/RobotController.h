#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QCoroTask>
#include <memory>

namespace core::sim { class RobotSimulator; }

namespace backend::controllers
{
    class RobotController : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("Managed by Backend")
        
        Q_PROPERTY(uint16_t jobId READ jobId NOTIFY stateChanged)
        Q_PROPERTY(uint8_t partType READ partType NOTIFY stateChanged)
        Q_PROPERTY(bool inMotion READ inMotion NOTIFY stateChanged)
        Q_PROPERTY(bool inHome READ inHome NOTIFY stateChanged)
        Q_PROPERTY(bool enabled READ enabled NOTIFY stateChanged)
        Q_PROPERTY(QString adsStatus READ adsStatus NOTIFY adsStatusChanged)

      public:
        explicit RobotController(std::shared_ptr<core::sim::RobotSimulator> simulator, QObject* parent = nullptr);
        
        uint16_t jobId() const;
        uint8_t partType() const;
        bool inMotion() const;
        bool inHome() const;
        bool enabled() const;
        QString adsStatus() const;

        Q_INVOKABLE void connectAds();

      signals:
        void stateChanged();
        void adsStatusChanged();

      private:
        std::shared_ptr<core::sim::RobotSimulator> m_simulator;
    };
}
