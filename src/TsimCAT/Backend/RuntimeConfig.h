#pragma once

#include "Link/LinkFactory.hpp"
#include "Simulators/ConveyorSimulator.hpp"

#include <QString>

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

    struct RuntimeConfig
    {
        std::string loggerFilePath;
        QString screenshotDirectory;

        core::link::LinkConfig tcpLink;
        core::link::LinkConfig adsLink;

        core::sim::ConveyorSimulator::Config entryConveyor;
        core::sim::ConveyorSimulator::Config exitConveyor;
        core::sim::ConveyorSimulator::Config transferConveyor;
        SimulationConfig simulation;
        AnalyzerConfig analyzer;

        static RuntimeConfig defaults();

        static RuntimeConfig loadFromFile(const QString& path, QString* diagnostics = nullptr);
    };
}
