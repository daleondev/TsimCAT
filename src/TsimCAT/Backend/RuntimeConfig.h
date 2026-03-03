#pragma once

#include "Link/LinkFactory.hpp"

#include <QString>
#include <string>
#include <vector>

namespace backend
{
    struct StationModeConfig
    {
        bool internal{ true };
    };

    struct CellFlowConfig
    {
        bool enabled{ true };
        bool autoStart{ true };
        double inspectionRejectRate{ 0.2 };
    };

    struct SimulationConfig
    {
        bool localOnly{ true };
        StationModeConfig camera;
        StationModeConfig laser;
        StationModeConfig robot;
        StationModeConfig gantry;
        StationModeConfig entryConveyor;
        StationModeConfig exitConveyor;
        CellFlowConfig cellFlow;
    };

    struct AnalyzerConfig
    {
        bool enabled{ false };
        bool autoStart{ true };
        bool saveFrames{ true };
        bool saveTrace{ true };
        int frameIntervalMs{ 250 };
        int traceIntervalMs{ 100 };
        int maxFrames{ 240 };
        QString outputFolder{ QStringLiteral("analysis/session") };
    };

    struct PathsConfig
    {
        QString logDirectory{ QStringLiteral("logs") };
        QString logFileName{ QStringLiteral("TsimCAT.log") };
        QString analysisDirectory{ QStringLiteral("analysis/session") };
    };

    struct TraceConfig
    {
        bool enabled{ true };
        bool enableProtocol{ true };
        bool enableState{ true };
        bool enableFlow{ true };
        bool enableInvariant{ true };
        bool mirrorToHumanLog{ true };
        int sampleIntervalMs{ 0 };
        QString outputFolder{ QStringLiteral("analysis/session") };
        QString fileName{ QStringLiteral("protocol_trace.jsonl") };
        std::vector<std::string> stationFilter;
    };

    struct AdsVariableConfig
    {
        struct Robot
        {
            std::string control{ "MAIN.stRobotControl" };
            std::string status{ "MAIN.stRobotStatus" };
        } robot;

        struct Conveyors
        {
            std::string entryRun{ "MAIN.bEntryConveyorRun" };
            std::vector<std::string> entrySensors{ "MAIN.bEntrySensor1",
                                                   "MAIN.bEntrySensor2",
                                                   "MAIN.bEntrySensor3" };

            std::string exitRun{ "MAIN.bExitConveyorRun" };
            std::vector<std::string> exitSensors{ "MAIN.bExitSensor1", "MAIN.bExitSensor2" };

            std::string transferRun{ "MAIN.bTransferConveyorRun" };
            std::vector<std::string> transferSensors{ "MAIN.bTransferSensor1",
                                                      "MAIN.bTransferSensor2",
                                                      "MAIN.bTransferSensor3" };
        } conveyors;

        struct Future
        {
            std::string cameraTrigger{ "MAIN.bCameraTrigger" };
            std::string cameraResult{ "MAIN.nCameraResult" };
            std::string laserStart{ "MAIN.bLaserStart" };
            std::string laserDone{ "MAIN.bLaserDone" };
            std::string gantryPosX{ "MAIN.fGantryPosX" };
            std::string gantryPosZ{ "MAIN.fGantryPosZ" };
            std::string gantryGripCmd{ "MAIN.bGantryGripCmd" };
            std::string gantryGripFb{ "MAIN.bGantryGripFb" };
            std::string safetyDoorClosed{ "MAIN.bSafetyDoorClosed" };
            std::string safetyEStopOk{ "MAIN.bEStopOk" };
        } future;
    };

    struct RuntimeConfig
    {
        PathsConfig paths;

        core::link::LinkConfig tcpLink;
        core::link::LinkConfig adsLink;
        SimulationConfig simulation;
        AnalyzerConfig analyzer;
        TraceConfig trace;
        AdsVariableConfig adsVariables;

        auto loggerFilePath() const -> std::string
        {
            return (paths.logDirectory + QLatin1Char('/') + paths.logFileName).toStdString();
        }

        static RuntimeConfig defaults();

        static RuntimeConfig loadFromFile(const QString& path, QString* diagnostics = nullptr);
    };
}
