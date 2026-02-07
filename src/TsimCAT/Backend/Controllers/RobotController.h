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
        
        // Status from Robot (Read-only for UI)
        Q_PROPERTY(uint16_t jobIdFeedback READ jobIdFeedback NOTIFY stateChanged)
        Q_PROPERTY(uint8_t partTypeMirrored READ partTypeMirrored NOTIFY stateChanged)
        Q_PROPERTY(bool inMotion READ inMotion NOTIFY stateChanged)
        Q_PROPERTY(bool inHome READ inHome NOTIFY stateChanged)
        Q_PROPERTY(bool enabled READ enabled NOTIFY stateChanged)
        Q_PROPERTY(bool error READ error NOTIFY stateChanged)
        Q_PROPERTY(bool brakeTestOk READ brakeTestOk NOTIFY stateChanged)
        Q_PROPERTY(bool masteringOk READ masteringOk NOTIFY stateChanged)
        Q_PROPERTY(bool inT1 READ inT1 NOTIFY stateChanged)
        Q_PROPERTY(bool inT2 READ inT2 NOTIFY stateChanged)
        Q_PROPERTY(bool inAut READ inAut NOTIFY stateChanged)
        Q_PROPERTY(bool inExt READ inExt NOTIFY stateChanged)
        Q_PROPERTY(uint8_t areaFreeRobot READ areaFreeRobot NOTIFY stateChanged)
        Q_PROPERTY(uint32_t errorCode READ errorCode NOTIFY stateChanged)

        // Joint Angles (Degrees)
        Q_PROPERTY(double axis1 READ axis1 NOTIFY stateChanged)
        Q_PROPERTY(double axis2 READ axis2 NOTIFY stateChanged)
        Q_PROPERTY(double axis3 READ axis3 NOTIFY stateChanged)
        Q_PROPERTY(double axis4 READ axis4 NOTIFY stateChanged)
        Q_PROPERTY(double axis5 READ axis5 NOTIFY stateChanged)
        Q_PROPERTY(double axis6 READ axis6 NOTIFY stateChanged)
        Q_PROPERTY(bool gripperGripped READ gripperGripped WRITE setGripperGripped NOTIFY stateChanged)

        // TCP Pose (X, Y, Z in mm; Roll, Pitch, Yaw in Degrees)
        Q_PROPERTY(double tcpX READ tcpX NOTIFY stateChanged)
        Q_PROPERTY(double tcpY READ tcpY NOTIFY stateChanged)
        Q_PROPERTY(double tcpZ READ tcpZ NOTIFY stateChanged)
        Q_PROPERTY(double tcpRoll READ tcpRoll NOTIFY stateChanged)
        Q_PROPERTY(double tcpPitch READ tcpPitch NOTIFY stateChanged)
        Q_PROPERTY(double tcpYaw READ tcpYaw NOTIFY stateChanged)

        // Control from PLC (Observing what PLC sends)
        Q_PROPERTY(uint16_t controlJobId READ controlJobId NOTIFY stateChanged)
        Q_PROPERTY(uint8_t controlPartType READ controlPartType NOTIFY stateChanged)
        Q_PROPERTY(bool controlMoveEnable READ controlMoveEnable NOTIFY stateChanged)
        Q_PROPERTY(bool controlReset READ controlReset NOTIFY stateChanged)
        Q_PROPERTY(uint8_t areaFreePlc READ areaFreePlc NOTIFY stateChanged)

        Q_PROPERTY(QString adsStatus READ adsStatus NOTIFY adsStatusChanged)

      public:
        explicit RobotController(std::shared_ptr<core::sim::RobotSimulator> simulator, QObject* parent = nullptr);
        
        uint16_t jobIdFeedback() const;
        uint8_t partTypeMirrored() const;
        bool inMotion() const;
        bool inHome() const;
        bool enabled() const;
        bool error() const;
        bool brakeTestOk() const;
        bool masteringOk() const;
        bool inT1() const;
        bool inT2() const;
        bool inAut() const;
        bool inExt() const;
        uint8_t areaFreeRobot() const;
        uint32_t errorCode() const;

        double axis1() const;
        double axis2() const;
        double axis3() const;
        double axis4() const;
        double axis5() const;
        double axis6() const;

        double tcpX() const;
        double tcpY() const;
        double tcpZ() const;
        double tcpRoll() const;
        double tcpPitch() const;
        double tcpYaw() const;

        bool gripperGripped() const;
        void setGripperGripped(bool gripped);

        uint16_t controlJobId() const;
        uint8_t controlPartType() const;
        bool controlMoveEnable() const;
        bool controlReset() const;
        uint8_t areaFreePlc() const;

        QString adsStatus() const;

        Q_INVOKABLE void connectAds();
        Q_INVOKABLE void setJoints(double j1, double j2, double j3, double j4, double j5, double j6);
        Q_INVOKABLE bool setTcp(double x, double y, double z, double r, double p, double w);

      signals:
        void stateChanged();
        void adsStatusChanged();

      private:
        std::shared_ptr<core::sim::RobotSimulator> m_simulator;
    };
}
