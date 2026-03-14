#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace core::sim
{
    class RobotSimulator;
    class RotaryTableSimulator;
    class ConveyorSimulator;
    class SimpleCellCoordinator;
}

namespace backend
{
    class ScreenshotProvider : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("Managed by Backend")
        Q_PROPERTY(bool capturePending READ capturePending NOTIFY capturePendingChanged)
        Q_PROPERTY(bool autoCaptureEnabled READ autoCaptureEnabled WRITE setAutoCaptureEnabled NOTIFY
                     autoCaptureEnabledChanged)
        Q_PROPERTY(int captureCount READ captureCount NOTIFY captureCountChanged)

      public:
        struct SimulatorRefs
        {
            std::shared_ptr<core::sim::RobotSimulator> robot;
            std::shared_ptr<core::sim::RotaryTableSimulator> rotaryTable;
            std::shared_ptr<core::sim::ConveyorSimulator> exitConveyor;
            std::shared_ptr<core::sim::SimpleCellCoordinator> coordinator;
        };

        explicit ScreenshotProvider(SimulatorRefs refs,
                                    const QString& outputFolder,
                                    int autoIntervalMs = 250,
                                    int maxFrames = 240,
                                    QObject* parent = nullptr);
        ~ScreenshotProvider() override;

        bool capturePending() const { return m_capturePending; }
        bool autoCaptureEnabled() const { return m_autoCaptureEnabled; }
        int captureCount() const { return m_captureCount; }

        void setAutoCaptureEnabled(bool enabled);

        Q_INVOKABLE void requestCapture();
        Q_INVOKABLE void onFrameCaptured(const QVariant& imageResult);

      signals:
        void capturePendingChanged();
        void autoCaptureEnabledChanged();
        void captureCountChanged();

      private:
        auto buildStateSnapshot() const -> QByteArray;
        auto ensureOutputFolder() -> bool;

        SimulatorRefs m_refs;
        QString m_outputFolder;
        int m_maxFrames;
        int m_captureCount{ 0 };
        bool m_capturePending{ false };
        bool m_autoCaptureEnabled{ false };
        QTimer* m_autoTimer{ nullptr };
    };
}
