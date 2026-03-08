#pragma once

#include "Link/LinkFactory.hpp"

#include <QString>
#include <string>
#include <vector>

namespace backend
{
    struct StationModeConfig
    {
        bool internal{ false };
    };

    struct StationPoseConfig
    {
        double x{ 0.0 };
        double y{ 0.0 };
        double z{ 0.0 };
        double rotationZ{ 0.0 };
    };

    struct PlantLayoutConfig
    {
        double floorScale{ 150.0 };
        StationPoseConfig rotaryTable{ .x = -1350.0, .y = 0.0, .z = 120.0, .rotationZ = 0.0 };
        StationPoseConfig robot{ .x = 0.0, .y = 0.0, .z = 350.0, .rotationZ = 90.0 };
        StationPoseConfig laser{ .x = 0.0, .y = 0.0, .z = -900.0, .rotationZ = 0.0 };
        StationPoseConfig exitConveyor{ .x = 1750.0, .y = 0.0, .z = 120.0, .rotationZ = 0.0 };
    };

    struct RotaryTableConfig
    {
        double radius{ 420.0 };
        double loadAngleDeg{ 180.0 };
        double pickAngleDeg{ 0.0 };
        double height{ 760.0 };
        double rotationSpeedDegPerSecond{ 95.0 };
        double loadDelaySeconds{ 1.0 };
    };

    struct ConveyorConfig
    {
        double length{ 1250.0 };
        double speed{ 250.0 };
        std::vector<double> sensorPositions{ 120.0, 420.0, 760.0, 1120.0 };
        int damperSensorIndex{ 1 };
        int damperCloseSensorIndex{ 2 };
        int endSensorIndex{ 3 };
        bool consumeAtEndSensor{ true };
        double damperOpenDelaySeconds{ 0.8 };
    };

    struct LocalCellConfig
    {
        bool enabled{ true };
        bool cyclePartTypes{ true };
        int markingDelayMs{ 900 };
        int idleLoadDelayMs{ 500 };
    };

    struct SimulationConfig
    {
        bool localOnly{ true };
        bool localPlcShadow{ true };
        StationModeConfig robot;
        StationModeConfig rotaryTable;
        StationModeConfig exitConveyor;
        RotaryTableConfig rotaryTableConfig;
        ConveyorConfig exitConveyorConfig;
        LocalCellConfig localCell;
        PlantLayoutConfig layout;
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
            std::string exitRun{ "MAIN.bExitConveyorRun" };
            std::vector<std::string> exitSensors{ "MAIN.bExitSensor1",
                                                  "MAIN.bExitSensor2",
                                                  "MAIN.bExitSensor3",
                                                  "MAIN.bExitSensor4" };
        } conveyors;

        struct RotaryTable
        {
            std::string control{ "MAIN.stRotaryTableControl" };
            std::string status{ "MAIN.stRotaryTableStatus" };
        } rotaryTable;
    };

    struct RuntimeConfig
    {
        PathsConfig paths;

        core::link::LinkConfig tcpLink;
        core::link::LinkConfig adsLink;
        core::link::LinkConfig opcUaLink;
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
