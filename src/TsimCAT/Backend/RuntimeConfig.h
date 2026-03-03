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

    struct SimulationConfig
    {
        bool localOnly{ true };
        StationModeConfig robot;
        StationModeConfig entryConveyor;
        StationModeConfig exitConveyor;
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
            std::vector<std::string> exitSensors{ "MAIN.bExitSensor1",
                                                  "MAIN.bExitSensor2",
                                                  "MAIN.bExitSensor3" };
        } conveyors;
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
