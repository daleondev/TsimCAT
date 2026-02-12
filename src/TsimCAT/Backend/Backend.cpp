#include "Backend.h"
#include "Controllers/LaserController.h"
#include "Controllers/RobotController.h"
#include "Controllers/ConveyorController.h"
#include "Link/LinkFactory.hpp"
#include "Logger/Logger.hpp"
#include "Simulators/LaserSimulator.hpp"
#include "Simulators/RobotSimulator.hpp"
#include "Simulators/ConveyorSimulator.hpp"
#include <QCoroTimer>
#include <QDateTime>
#include <QDir>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <chrono>

using namespace std::chrono_literals;

namespace backend
{
    Backend::Backend(QObject* parent)
      : QObject(parent)
    {
        // Initialize logger
        core::logger::Logger::instance().init("logs/TsimCAT.log");
        core::logger::info("Backend initialized");

        // 1. Create Shared Links
        core::link::LinkConfig tcpConfig{};
        tcpConfig.port = 12345;
        auto tcpRes = core::link::create(
          core::link::Role::Server, core::link::Mode::Raw, core::link::Protocol::Tcp, tcpConfig);
        if (tcpRes) {
            m_tcpLink = std::move(*tcpRes);
        }
        else {
            core::logger::error("Failed to create shared TCP link: {}", tcpRes.error().message());
        }

        core::link::LinkConfig adsConfig{};
        adsConfig.ip = "127.0.0.1";
        adsConfig.remoteNetId = "192.168.167.100.1.1";
        adsConfig.port = 851;
        adsConfig.localNetId = "192.168.167.100.1.20";
        auto adsRes = core::link::create(
          core::link::Role::Client, core::link::Mode::Symbolic, core::link::Protocol::Ads, adsConfig);
        if (adsRes) {
            m_adsLink = std::move(*adsRes);
        }
        else {
            core::logger::error("Failed to create shared ADS link: {}", adsRes.error().message());
        }

        // 2. Create Simulators
        m_laserSim = std::make_shared<core::sim::LaserSimulator>(m_tcpLink);
        m_robotSim = std::make_shared<core::sim::RobotSimulator>(m_adsLink);

        core::sim::ConveyorSimulator::Config entryConfig{
            .name = "EntryConveyor",
            .length = 1875.0,
            .speed = 250.0,
            .sensorPositions = { 437.5, 1000.0, 1775.0 }, // Sensor 1 is at damper
            .damperSensorIndex = 0,
            .endSensorIndex = 2,
            .adsRunCmd = "MAIN.bEntryConveyorRun",
            .adsSensorSignals = { "MAIN.bEntrySensor1", "MAIN.bEntrySensor2", "MAIN.bEntrySensor3" }
        };
        m_entryConveyorSim = std::make_shared<core::sim::ConveyorSimulator>(entryConfig, m_adsLink);

        core::sim::ConveyorSimulator::Config exitConfig{
            .name = "ExitConveyor",
            .length = 1250.0,
            .speed = 250.0,
            .sensorPositions = { 100.0, 1150.0 },
            .endSensorIndex = 1,
            .adsRunCmd = "MAIN.bExitConveyorRun",
            .adsSensorSignals = { "MAIN.bExitSensor1", "MAIN.bExitSensor2" }
        };
        m_exitConveyorSim = std::make_shared<core::sim::ConveyorSimulator>(exitConfig, m_adsLink);

        m_entryConveyorSim->start();
        m_exitConveyorSim->start();

        // Start ADS Sync tasks for conveyors
        if (m_entryConveyorSim) {
            [](std::shared_ptr<core::sim::ConveyorSimulator> sim) -> core::coro::Task<void> {
                co_await sim->run();
            }(m_entryConveyorSim);
        }
        if (m_exitConveyorSim) {
            [](std::shared_ptr<core::sim::ConveyorSimulator> sim) -> core::coro::Task<void> {
                co_await sim->run();
            }(m_exitConveyorSim);
        }

        // 3. Inject into Controllers
        m_laserController = std::make_unique<backend::controllers::LaserController>(m_laserSim, this);
        m_robotController = std::make_unique<backend::controllers::RobotController>(m_robotSim, this);
        m_entryConveyorController = std::make_unique<backend::controllers::ConveyorController>(m_entryConveyorSim, this);
        m_exitConveyorController = std::make_unique<backend::controllers::ConveyorController>(m_exitConveyorSim, this);

        // 4. Simulation Loop (10ms ~ 100Hz)
        [] (Backend* self) -> QCoro::Task<void> {
            auto lastTime = std::chrono::steady_clock::now();
            while (true) {
                auto now = std::chrono::steady_clock::now();
                double dt = std::chrono::duration<double>(now - lastTime).count();
                lastTime = now;

                if (self->m_laserSim) self->m_laserSim->update(dt);
                if (self->m_robotSim) self->m_robotSim->update(dt);
                if (self->m_entryConveyorSim) self->m_entryConveyorSim->update(dt);
                if (self->m_exitConveyorSim) self->m_exitConveyorSim->update(dt);

                // Notify UI about state changes
                if (self->m_entryConveyorController) emit self->m_entryConveyorController->stateChanged();
                if (self->m_exitConveyorController) emit self->m_exitConveyorController->stateChanged();

                co_await QCoro::sleepFor(10ms);
            }
        }(this);
    }

    Backend::~Backend() = default;

    QString Backend::welcomeMessage() const { return QStringLiteral("Hello from C++ Backend!"); }

    QString Backend::asyncTestStatus() const { return m_asyncTestStatus; }

    backend::controllers::LaserController* Backend::laser() const { return m_laserController.get(); }

    backend::controllers::RobotController* Backend::robot() const { return m_robotController.get(); }

    backend::controllers::ConveyorController* Backend::entryConveyor() const { return m_entryConveyorController.get(); }

    backend::controllers::ConveyorController* Backend::exitConveyor() const { return m_exitConveyorController.get(); }

    void Backend::runAsyncTest() { doAsyncTest(); }

    void Backend::captureScreenshot(QObject* item, const QString& filename)
    {
        auto* quickItem = qobject_cast<QQuickItem*>(item);
        if (!quickItem) {
            core::logger::error("captureScreenshot: Item is null or not a QQuickItem");
            return;
        }

        auto result = quickItem->grabToImage();
        if (result) {
            connect(result.data(), &QQuickItemGrabResult::ready, this, [this, result, filename]() {
                QDir dir("screenshots");
                if (!dir.exists())
                    dir.mkpath(".");

                QString finalFilename;
                if (filename.isEmpty()) {
                    finalFilename = QString("screenshots/capture_%1.png")
                                         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));
                } else {
                    finalFilename = QString("screenshots/%1.png").arg(filename);
                }

                if (result->saveToFile(finalFilename)) {
                    core::logger::info("Screenshot saved to {}", finalFilename.toStdString());
                }
                else {
                    core::logger::error("Failed to save screenshot to {}", finalFilename.toStdString());
                }
            });
        }
    }

    QCoro::Task<void> Backend::doAsyncTest()
    {
        m_asyncTestStatus = "Testing QCoro... (Waiting 2s)";
        emit asyncTestStatusChanged();

        co_await QCoro::sleepFor(2s);

        m_asyncTestStatus = "QCoro Works! (Delay finished)";
        emit asyncTestStatusChanged();

        co_return;
    }
}
