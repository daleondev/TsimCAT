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

    }

    RuntimeConfig RuntimeConfig::defaults()
    {
        RuntimeConfig config{};

        config.paths.logDirectory = QStringLiteral("logs");
        config.paths.logFileName = QStringLiteral("TsimCAT.log");
        config.paths.analysisDirectory = QStringLiteral("analysis/session");

        config.tcpLink.port = 12345;

        config.adsLink.ip = "127.0.0.1";
        config.adsLink.remoteNetId = "192.168.167.100.1.1";
        config.adsLink.localNetId = "192.168.167.100.1.20";
        config.adsLink.port = 851;

        config.simulation.localOnly = false;
        config.simulation.laser.internal = true;
        config.simulation.robot.internal = true;
        config.simulation.gantry.internal = true;
        config.simulation.entryConveyor.internal = true;
        config.simulation.exitConveyor.internal = true;
        config.simulation.cellFlow.enabled = true;
        config.simulation.cellFlow.autoStart = true;
        config.simulation.cellFlow.inspectionRejectRate = 0.2;

        config.analyzer.enabled = false;
        config.analyzer.autoStart = true;
        config.analyzer.saveFrames = true;
        config.analyzer.saveTrace = true;
        config.analyzer.frameIntervalMs = 250;
        config.analyzer.traceIntervalMs = 100;
        config.analyzer.maxFrames = 240;
        config.analyzer.outputFolder = config.paths.analysisDirectory;

        config.trace.enabled = true;
        config.trace.enableProtocol = true;
        config.trace.enableState = true;
        config.trace.enableFlow = true;
        config.trace.enableInvariant = true;
        config.trace.mirrorToHumanLog = true;
        config.trace.sampleIntervalMs = 0;
        config.trace.outputFolder = config.paths.analysisDirectory;
        config.trace.fileName = QStringLiteral("protocol_trace.jsonl");

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

        const auto paths = asObject(root, "paths");
        applyString(paths, "logDirectory", config.paths.logDirectory);
        applyString(paths, "logFileName", config.paths.logFileName);
        applyString(paths, "analysisDirectory", config.paths.analysisDirectory);
        config.analyzer.outputFolder = config.paths.analysisDirectory;
        config.trace.outputFolder = config.paths.analysisDirectory;

        const auto links = asObject(root, "links");
        const auto tcp = asObject(links, "tcp");
        applyUInt16(tcp, "port", config.tcpLink.port);

        const auto ads = asObject(links, "ads");
        applyString(ads, "ip", config.adsLink.ip);
        applyString(ads, "remoteNetId", config.adsLink.remoteNetId);
        applyString(ads, "localNetId", config.adsLink.localNetId);
        applyUInt16(ads, "port", config.adsLink.port);

        const auto adsVariables = asObject(root, "adsVariables");
        const auto adsRobot = asObject(adsVariables, "robot");
        applyString(adsRobot, "control", config.adsVariables.robot.control);
        applyString(adsRobot, "status", config.adsVariables.robot.status);

        const auto adsConveyors = asObject(adsVariables, "conveyors");
        applyString(adsConveyors, "entryRun", config.adsVariables.conveyors.entryRun);
        applyStringArray(adsConveyors, "entrySensors", config.adsVariables.conveyors.entrySensors);
        applyString(adsConveyors, "exitRun", config.adsVariables.conveyors.exitRun);
        applyStringArray(adsConveyors, "exitSensors", config.adsVariables.conveyors.exitSensors);
        applyString(adsConveyors, "transferRun", config.adsVariables.conveyors.transferRun);
        applyStringArray(adsConveyors, "transferSensors", config.adsVariables.conveyors.transferSensors);

        const auto adsFuture = asObject(adsVariables, "future");
        applyString(adsFuture, "cameraTrigger", config.adsVariables.future.cameraTrigger);
        applyString(adsFuture, "cameraResult", config.adsVariables.future.cameraResult);
        applyString(adsFuture, "laserStart", config.adsVariables.future.laserStart);
        applyString(adsFuture, "laserDone", config.adsVariables.future.laserDone);
        applyString(adsFuture, "gantryPosX", config.adsVariables.future.gantryPosX);
        applyString(adsFuture, "gantryPosZ", config.adsVariables.future.gantryPosZ);
        applyString(adsFuture, "gantryGripCmd", config.adsVariables.future.gantryGripCmd);
        applyString(adsFuture, "gantryGripFb", config.adsVariables.future.gantryGripFb);
        applyString(adsFuture, "safetyDoorClosed", config.adsVariables.future.safetyDoorClosed);
        applyString(adsFuture, "safetyEStopOk", config.adsVariables.future.safetyEStopOk);

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

        const auto analyzer = asObject(root, "analyzer");
        applyBool(analyzer, "enabled", config.analyzer.enabled);
        applyBool(analyzer, "autoStart", config.analyzer.autoStart);
        applyBool(analyzer, "saveFrames", config.analyzer.saveFrames);
        applyBool(analyzer, "saveTrace", config.analyzer.saveTrace);
        applyInt(analyzer, "frameIntervalMs", config.analyzer.frameIntervalMs);
        applyInt(analyzer, "traceIntervalMs", config.analyzer.traceIntervalMs);
        applyInt(analyzer, "maxFrames", config.analyzer.maxFrames);
        applyString(analyzer, "outputFolder", config.analyzer.outputFolder);

        if (config.analyzer.outputFolder.isEmpty()) {
            config.analyzer.outputFolder = config.paths.analysisDirectory;
        }

        const auto trace = asObject(root, "trace");
        applyBool(trace, "enabled", config.trace.enabled);
        applyBool(trace, "enableProtocol", config.trace.enableProtocol);
        applyBool(trace, "enableState", config.trace.enableState);
        applyBool(trace, "enableFlow", config.trace.enableFlow);
        applyBool(trace, "enableInvariant", config.trace.enableInvariant);
        applyBool(trace, "mirrorToHumanLog", config.trace.mirrorToHumanLog);
        applyInt(trace, "sampleIntervalMs", config.trace.sampleIntervalMs);
        applyString(trace, "outputFolder", config.trace.outputFolder);
        applyString(trace, "fileName", config.trace.fileName);
        applyStringArray(trace, "stationFilter", config.trace.stationFilter);

        if (config.trace.outputFolder.isEmpty()) {
            config.trace.outputFolder = config.paths.analysisDirectory;
        }
        if (config.trace.fileName.isEmpty()) {
            config.trace.fileName = QStringLiteral("protocol_trace.jsonl");
        }

        if (diagnostics) {
            *diagnostics = QStringLiteral("Loaded runtime config from '%1'.").arg(path);
        }

        return config;
    }
}
