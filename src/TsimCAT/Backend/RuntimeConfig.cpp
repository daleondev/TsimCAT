#include "RuntimeConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace backend
{
    namespace
    {
        auto makeRobotPose(double x, double y, double z, double rollDeg, double pitchDeg, double yawDeg)
          -> RobotPoseConfig
        {
            return RobotPoseConfig{
                .x = x, .y = y, .z = z, .rollDeg = rollDeg, .pitchDeg = pitchDeg, .yawDeg = yawDeg
            };
        }

        auto defaultRobotJobs() -> std::vector<RobotJobConfig>
        {
            return {
                                RobotJobConfig{ .jobId = 1,
                                                                .name = "Home",
                                                                .poses = { makeRobotPose(395.0, 0.0, 765.0, 0.0, 90.0, 0.0) } },
                RobotJobConfig{ .jobId = 2,
                                .name = "PickEntry",
                                                                .poses = { makeRobotPose(395.0, 0.0, 765.0, 0.0, 90.0, 0.0),
                                                                                     makeRobotPose(615.0, 650.0, 520.0, 0.0, 0.0, 90.0),
                                                                                     makeRobotPose(395.0, 0.0, 765.0, 0.0, 90.0, 0.0) } },
                RobotJobConfig{ .jobId = 3,
                                .name = "PlaceLaser",
                                .poses = { makeRobotPose(395.0, 0.0, 765.0, 0.0, 90.0, 0.0),
                                           makeRobotPose(520.0, 0.0, 760.0, 0.0, 0.0, 0.0),
                                           makeRobotPose(670.0, 0.0, 630.0, 0.0, 0.0, 0.0),
                                           makeRobotPose(520.0, 0.0, 760.0, 0.0, 0.0, 0.0),
                                           makeRobotPose(395.0, 0.0, 765.0, 0.0, 90.0, 0.0) } },
                RobotJobConfig{ .jobId = 4,
                                .name = "PickLaser",
                                                                .poses = { makeRobotPose(520.0, 0.0, 760.0, 0.0, 0.0, 0.0),
                                                                                     makeRobotPose(670.0, 0.0, 630.0, 0.0, 0.0, 0.0),
                                                                                     makeRobotPose(670.0, 0.0, 565.0, 0.0, 0.0, 0.0),
                                                                                     makeRobotPose(670.0, 0.0, 630.0, 0.0, 0.0, 0.0),
                                           makeRobotPose(520.0, 0.0, 760.0, 0.0, 0.0, 0.0),
                                                                                     } },
                RobotJobConfig{ .jobId = 7,
                                .name = "PlaceExit",
                                                                .poses = { makeRobotPose(395.0, 0.0, 765.0, 0.0, 90.0, 0.0),
                                                                                     makeRobotPose(615.0, -690.0, 520.0, 0.0, 0.0, -90.0),
                                                                                     makeRobotPose(395.0, 0.0, 765.0, 0.0, 90.0, 0.0) } },
            };
        }

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

        auto parseRobotPose(const QJsonObject& object, RobotPoseConfig& target) -> bool
        {
            const auto x = object.value(QLatin1StringView("x"));
            const auto y = object.value(QLatin1StringView("y"));
            const auto z = object.value(QLatin1StringView("z"));
            if (!x.isDouble() || !y.isDouble() || !z.isDouble()) {
                return false;
            }

            target = {};
            target.x = x.toDouble();
            target.y = y.toDouble();
            target.z = z.toDouble();
            applyDouble(object, "rollDeg", target.rollDeg);
            applyDouble(object, "pitchDeg", target.pitchDeg);
            applyDouble(object, "yawDeg", target.yawDeg);
            return true;
        }

        void applyRobotJobs(const QJsonObject& object, const char* key, std::vector<RobotJobConfig>& target)
        {
            const auto value = object.value(QLatin1StringView(key));
            if (!value.isArray()) {
                return;
            }

            std::vector<RobotJobConfig> jobs;
            const auto array = value.toArray();
            jobs.reserve(static_cast<size_t>(array.size()));

            for (const auto& item : array) {
                if (!item.isObject()) {
                    continue;
                }

                const auto jobObject = item.toObject();
                RobotJobConfig job;
                applyUInt16(jobObject, "jobId", job.jobId);
                applyString(jobObject, "name", job.name);

                const auto posesValue = jobObject.value(QLatin1StringView("poses"));
                if (!posesValue.isArray()) {
                    continue;
                }

                const auto posesArray = posesValue.toArray();
                std::vector<RobotPoseConfig> poses;
                poses.reserve(static_cast<size_t>(posesArray.size()));
                bool poseParseFailed = false;
                for (const auto& poseItem : posesArray) {
                    if (!poseItem.isObject()) {
                        poseParseFailed = true;
                        break;
                    }

                    RobotPoseConfig pose;
                    if (!parseRobotPose(poseItem.toObject(), pose)) {
                        poseParseFailed = true;
                        break;
                    }
                    poses.push_back(pose);
                }

                if (poseParseFailed || job.jobId == 0 || poses.empty()) {
                    continue;
                }

                job.poses = std::move(poses);
                jobs.push_back(std::move(job));
            }

            if (!jobs.empty()) {
                target = std::move(jobs);
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
        config.adsLink.inProcess = true;
        config.adsLink.instanceName = "simple_cell_local_ads";
        config.opcUaLink.ip = "opc.tcp://127.0.0.1:4840";

        config.simulation.localOnly = true;
        config.simulation.localPlcShadow = true;
        config.simulation.robot.internal = false;
        config.simulation.rotaryTable.internal = false;
        config.simulation.exitConveyor.internal = false;
        config.simulation.robotMotion.jobs = defaultRobotJobs();

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
        applyBool(ads, "inProcess", config.adsLink.inProcess);
        applyString(ads, "instanceName", config.adsLink.instanceName);

        const auto opcUa = asObject(links, "opcUa");
        applyString(opcUa, "endpoint", config.opcUaLink.ip);
        applyUInt16(opcUa, "port", config.opcUaLink.port);

        const auto adsVariables = asObject(root, "adsVariables");
        const auto adsRobot = asObject(adsVariables, "robot");
        applyString(adsRobot, "control", config.adsVariables.robot.control);
        applyString(adsRobot, "status", config.adsVariables.robot.status);

        const auto adsConveyors = asObject(adsVariables, "conveyors");
        applyString(adsConveyors, "exitRun", config.adsVariables.conveyors.exitRun);
        applyStringArray(adsConveyors, "exitSensors", config.adsVariables.conveyors.exitSensors);

        const auto adsRotaryTable = asObject(adsVariables, "rotaryTable");
        applyString(adsRotaryTable, "control", config.adsVariables.rotaryTable.control);
        applyString(adsRotaryTable, "status", config.adsVariables.rotaryTable.status);

        const auto adsLaser = asObject(adsVariables, "laser");
        applyString(adsLaser, "partPresentSensor", config.adsVariables.laser.partPresentSensor);

        const auto adsGripper = asObject(adsVariables, "gripper");
        applyString(adsGripper, "partDetectedSensor", config.adsVariables.gripper.partDetectedSensor);

        const auto simulation = asObject(root, "simulation");
        applyBool(simulation, "localOnly", config.simulation.localOnly);
        applyBool(simulation, "localPlcShadow", config.simulation.localPlcShadow);

        const auto stationModes = asObject(simulation, "stationModes");
        applyBool(stationModes, "robotInternal", config.simulation.robot.internal);
        applyBool(stationModes, "rotaryTableInternal", config.simulation.rotaryTable.internal);
        applyBool(stationModes, "exitConveyorInternal", config.simulation.exitConveyor.internal);
        applyRobotJobs(simulation, "robotJobs", config.simulation.robotMotion.jobs);

        const auto rotaryTable = asObject(simulation, "rotaryTable");
        applyDouble(rotaryTable, "radius", config.simulation.rotaryTableConfig.radius);
        applyDouble(rotaryTable, "height", config.simulation.rotaryTableConfig.height);
        applyDouble(rotaryTable, "loadAngleDeg", config.simulation.rotaryTableConfig.loadAngleDeg);
        applyDouble(rotaryTable, "pickAngleDeg", config.simulation.rotaryTableConfig.pickAngleDeg);
        applyDouble(rotaryTable,
                    "rotationSpeedDegPerSecond",
                    config.simulation.rotaryTableConfig.rotationSpeedDegPerSecond);
        applyDouble(rotaryTable, "loadDelaySeconds", config.simulation.rotaryTableConfig.loadDelaySeconds);

        const auto exitConveyor = asObject(simulation, "exitConveyor");
        applyDouble(exitConveyor, "length", config.simulation.exitConveyorConfig.length);
        applyDouble(exitConveyor, "speed", config.simulation.exitConveyorConfig.speed);
        applyDoubleArray(
          exitConveyor, "sensorPositions", config.simulation.exitConveyorConfig.sensorPositions);
        applyInt(exitConveyor, "damperSensorIndex", config.simulation.exitConveyorConfig.damperSensorIndex);
        applyInt(exitConveyor,
                 "damperCloseSensorIndex",
                 config.simulation.exitConveyorConfig.damperCloseSensorIndex);
        applyInt(exitConveyor, "endSensorIndex", config.simulation.exitConveyorConfig.endSensorIndex);
        applyBool(
          exitConveyor, "consumeAtEndSensor", config.simulation.exitConveyorConfig.consumeAtEndSensor);
        applyDouble(exitConveyor,
                    "damperOpenDelaySeconds",
                    config.simulation.exitConveyorConfig.damperOpenDelaySeconds);

        const auto localCell = asObject(simulation, "localCell");
        applyBool(localCell, "enabled", config.simulation.localCell.enabled);
        applyBool(localCell, "cyclePartTypes", config.simulation.localCell.cyclePartTypes);
        applyInt(localCell, "markingDelayMs", config.simulation.localCell.markingDelayMs);
        applyInt(localCell, "idleLoadDelayMs", config.simulation.localCell.idleLoadDelayMs);

        const auto layout = asObject(simulation, "layout");
        applyDouble(layout, "floorScale", config.simulation.layout.floorScale);

        const auto layoutRotary = asObject(layout, "rotaryTable");
        applyDouble(layoutRotary, "x", config.simulation.layout.rotaryTable.x);
        applyDouble(layoutRotary, "y", config.simulation.layout.rotaryTable.y);
        applyDouble(layoutRotary, "z", config.simulation.layout.rotaryTable.z);
        applyDouble(layoutRotary, "rotationZ", config.simulation.layout.rotaryTable.rotationZ);

        const auto layoutRobot = asObject(layout, "robot");
        applyDouble(layoutRobot, "x", config.simulation.layout.robot.x);
        applyDouble(layoutRobot, "y", config.simulation.layout.robot.y);
        applyDouble(layoutRobot, "z", config.simulation.layout.robot.z);
        applyDouble(layoutRobot, "rotationZ", config.simulation.layout.robot.rotationZ);

        const auto layoutLaser = asObject(layout, "laser");
        applyDouble(layoutLaser, "x", config.simulation.layout.laser.x);
        applyDouble(layoutLaser, "y", config.simulation.layout.laser.y);
        applyDouble(layoutLaser, "z", config.simulation.layout.laser.z);
        applyDouble(layoutLaser, "rotationZ", config.simulation.layout.laser.rotationZ);

        const auto layoutExitConveyor = asObject(layout, "exitConveyor");
        applyDouble(layoutExitConveyor, "x", config.simulation.layout.exitConveyor.x);
        applyDouble(layoutExitConveyor, "y", config.simulation.layout.exitConveyor.y);
        applyDouble(layoutExitConveyor, "z", config.simulation.layout.exitConveyor.z);
        applyDouble(layoutExitConveyor, "rotationZ", config.simulation.layout.exitConveyor.rotationZ);

        if (config.simulation.localOnly || config.simulation.localPlcShadow) {
            config.adsLink.inProcess = true;
            if (config.adsLink.instanceName.empty()) {
                config.adsLink.instanceName = "simple_cell_local_ads";
            }
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
