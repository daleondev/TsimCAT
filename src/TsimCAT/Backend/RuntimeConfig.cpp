#include "RuntimeConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace backend
{
    namespace
    {
        auto asObject(const QJsonObject& object, const char* key) -> QJsonObject
        {
            const auto value = object.value(QLatin1StringView(key));
            return value.isObject() ? value.toObject() : QJsonObject{};
        }

        void applyString(const QJsonObject& object, const char* key, std::string& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (value.isString()) {
                target = value.toString().toStdString();
            }
        }

        void applyString(const QJsonObject& object, const char* key, QString& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (value.isString()) {
                target = value.toString();
            }
        }

        void applyUInt16(const QJsonObject& object, const char* key, uint16_t& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (value.isDouble()) {
                const int parsed = value.toInt(static_cast<int>(target));
                if (parsed >= 0 && parsed <= 65535) {
                    target = static_cast<uint16_t>(parsed);
                }
            }
        }

        void applyInt(const QJsonObject& object, const char* key, int& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (value.isDouble()) {
                target = value.toInt(target);
            }
        }

        void applyDouble(const QJsonObject& object, const char* key, double& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (value.isDouble()) {
                target = value.toDouble(target);
            }
        }

        void applyBool(const QJsonObject& object, const char* key, bool& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (value.isBool()) {
                target = value.toBool(target);
            }
        }

        void applyDoubleArray(const QJsonObject& object, const char* key, std::vector<double>& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (!value.isArray()) {
                return;
            }

            std::vector<double> values;
            const auto array = value.toArray();
            values.reserve(static_cast<size_t>(array.size()));
            for (const auto& item : array) {
                if (!item.isDouble()) {
                    return;
                }
                values.push_back(item.toDouble());
            }

            if (!values.empty()) {
                target = std::move(values);
            }
        }

        void applyStringArray(const QJsonObject& object, const char* key, std::vector<std::string>& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (!value.isArray()) {
                return;
            }

            std::vector<std::string> values;
            const auto array = value.toArray();
            values.reserve(static_cast<size_t>(array.size()));
            for (const auto& item : array) {
                if (!item.isString()) {
                    return;
                }
                values.push_back(item.toString().toStdString());
            }

            if (!values.empty()) {
                target = std::move(values);
            }
        }

        void applyConveyor(const QJsonObject& object, core::sim::ConveyorSimulator::Config& config)
        {
            applyDouble(object, "length", config.length);
            applyDouble(object, "speed", config.speed);
            applyDoubleArray(object, "sensorPositions", config.sensorPositions);
            applyInt(object, "damperSensorIndex", config.damperSensorIndex);
            applyInt(object, "endSensorIndex", config.endSensorIndex);
            applyString(object, "adsRunCmd", config.adsRunCmd);
            applyStringArray(object, "adsSensorSignals", config.adsSensorSignals);
        }
    }

    RuntimeConfig RuntimeConfig::defaults()
    {
        RuntimeConfig config{};

        config.loggerFilePath = "logs/TsimCAT.log";
        config.screenshotDirectory = QStringLiteral("screenshots");

        config.tcpLink.port = 12345;

        config.adsLink.ip = "127.0.0.1";
        config.adsLink.remoteNetId = "192.168.167.100.1.1";
        config.adsLink.localNetId = "192.168.167.100.1.20";
        config.adsLink.port = 851;

        config.entryConveyor = { .name = "EntryConveyor",
                                 .length = 1875.0,
                                 .speed = 250.0,
                                 .sensorPositions = { 437.5, 1000.0, 1775.0 },
                                 .damperSensorIndex = 0,
                                 .endSensorIndex = 2,
                                 .adsRunCmd = "MAIN.bEntryConveyorRun",
                                 .adsSensorSignals = {
                                   "MAIN.bEntrySensor1", "MAIN.bEntrySensor2", "MAIN.bEntrySensor3" } };

        config.exitConveyor = { .name = "ExitConveyor",
                                .length = 1250.0,
                                .speed = 250.0,
                                .sensorPositions = { 100.0, 1150.0 },
                                .damperSensorIndex = -1,
                                .endSensorIndex = 1,
                                .adsRunCmd = "MAIN.bExitConveyorRun",
                                .adsSensorSignals = { "MAIN.bExitSensor1", "MAIN.bExitSensor2" } };

        config.transferConveyor = {
            .name = "TransferConveyor",
            .length = 1250.0,
            .speed = 250.0,
            .sensorPositions = { 120.0, 650.0, 1120.0 },
            .damperSensorIndex = 0,
            .endSensorIndex = 2,
            .adsRunCmd = "MAIN.bTransferConveyorRun",
            .adsSensorSignals = { "MAIN.bTransferSensor1", "MAIN.bTransferSensor2", "MAIN.bTransferSensor3" }
        };

        config.simulation.localOnly = false;
        config.simulation.laser.internal = true;
        config.simulation.robot.internal = true;
        config.simulation.gantry.internal = true;
        config.simulation.entryConveyor.internal = true;
        config.simulation.exitConveyor.internal = true;
        config.simulation.cellFlow.enabled = true;
        config.simulation.cellFlow.autoStart = true;
        config.simulation.cellFlow.inspectionRejectRate = 0.2;

        return config;
    }

    RuntimeConfig RuntimeConfig::loadFromFile(const QString& path, QString* diagnostics)
    {
        auto config = RuntimeConfig::defaults();

        QFile file(path);
        if (!file.exists()) {
            if (diagnostics) {
                *diagnostics = QStringLiteral("Runtime config '%1' not found, using defaults.").arg(path);
            }
            return config;
        }

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (diagnostics) {
                *diagnostics =
                  QStringLiteral("Failed to open runtime config '%1', using defaults.").arg(path);
            }
            return config;
        }

        const auto doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) {
            if (diagnostics) {
                *diagnostics =
                  QStringLiteral("Runtime config '%1' is invalid JSON object, using defaults.").arg(path);
            }
            return config;
        }

        const auto root = doc.object();

        const auto logging = asObject(root, "logging");
        applyString(logging, "file", config.loggerFilePath);

        const auto ui = asObject(root, "ui");
        applyString(ui, "screenshotDirectory", config.screenshotDirectory);

        const auto links = asObject(root, "links");
        const auto tcp = asObject(links, "tcp");
        applyUInt16(tcp, "port", config.tcpLink.port);

        const auto ads = asObject(links, "ads");
        applyString(ads, "ip", config.adsLink.ip);
        applyString(ads, "remoteNetId", config.adsLink.remoteNetId);
        applyString(ads, "localNetId", config.adsLink.localNetId);
        applyUInt16(ads, "port", config.adsLink.port);

        const auto conveyors = asObject(root, "conveyors");
        const auto entry = asObject(conveyors, "entry");
        applyConveyor(entry, config.entryConveyor);

        const auto exit = asObject(conveyors, "exit");
        applyConveyor(exit, config.exitConveyor);

        const auto transfer = asObject(conveyors, "transfer");
        applyConveyor(transfer, config.transferConveyor);

        const auto simulation = asObject(root, "simulation");
        applyBool(simulation, "localOnly", config.simulation.localOnly);

        const auto stationModes = asObject(simulation, "stationModes");
        applyBool(stationModes, "laserInternal", config.simulation.laser.internal);
        applyBool(stationModes, "robotInternal", config.simulation.robot.internal);
        applyBool(stationModes, "gantryInternal", config.simulation.gantry.internal);
        applyBool(stationModes, "entryConveyorInternal", config.simulation.entryConveyor.internal);
        applyBool(stationModes, "exitConveyorInternal", config.simulation.exitConveyor.internal);

        const auto cellFlow = asObject(simulation, "cellFlow");
        applyBool(cellFlow, "enabled", config.simulation.cellFlow.enabled);
        applyBool(cellFlow, "autoStart", config.simulation.cellFlow.autoStart);
        applyDouble(cellFlow, "inspectionRejectRate", config.simulation.cellFlow.inspectionRejectRate);

        if (config.simulation.localOnly) {
            config.simulation.laser.internal = true;
            config.simulation.robot.internal = true;
            config.simulation.gantry.internal = true;
            config.simulation.entryConveyor.internal = true;
            config.simulation.exitConveyor.internal = true;
        }

        if (diagnostics) {
            *diagnostics = QStringLiteral("Loaded runtime config from '%1'.").arg(path);
        }

        return config;
    }
}
