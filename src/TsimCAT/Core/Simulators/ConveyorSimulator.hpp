#pragma once

#include "ISimulator.hpp"
#include "Link/ILink.hpp"
#include "Part.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace core::sim
{
    class ConveyorSimulator : public ISimulator
    {
      public:
        struct Config
        {
            std::string name;
            double length;                       // mm
            double speed;                        // mm/s
            std::vector<double> sensorPositions; // mm from start

            // Logic config
            int damperSensorIndex{ -1 };      // Index in sensorPositions where damper is
            int damperCloseSensorIndex{ -1 }; // Optional close trigger sensor index
            int endSensorIndex{ -1 };         // Index in sensorPositions where end is
            bool consumeAtEndSensor{ false }; // Remove parts when they reach end sensor
            double damperOpenDelaySeconds{ 1.0 };

            // ADS Mapping
            std::string adsRunCmd;                     // e.g., "MAIN.bEntryRun"
            std::vector<std::string> adsSensorSignals; // e.g., {"MAIN.bEntryS1", ...}
        };

        explicit ConveyorSimulator(Config config, std::shared_ptr<link::ILink> link = nullptr);
        ~ConveyorSimulator() override;

        auto name() const -> std::string override { return m_config.name; }
        auto initialize() -> coro::Task<result::Result<void>> override;
        auto start() -> void override;
        auto stop() -> void override;
        auto update(double deltaTimeSeconds) -> void override;

        auto run() -> coro::Task<void>;

        // Simulation control
        auto spawnPart(uint8_t type) -> void;
        auto spawnPartAtPosition(uint8_t type, double position) -> void;
        auto clearParts() -> void;
        auto peekPartAtEnd() const -> std::optional<Part>;
        auto takePartAtEnd() -> std::optional<Part>;

        // State Access
        auto parts() const -> std::vector<Part>;
        auto sensors() const -> std::vector<bool>;
        auto speed() const -> double { return m_config.speed; }
        auto setSpeed(double speed) -> void { m_config.speed = speed; }
        auto length() const -> double { return m_config.length; }

        auto isRunning() const -> bool { return m_beltRunning; }
        auto setRunning(bool running) -> void { m_beltRunning = running; }

        auto autoSpawn() const -> bool { return m_autoSpawn; }
        auto setAutoSpawn(bool enable) -> void { m_autoSpawn = enable; }

        auto autoLogic() const -> bool { return m_autoLogic; }
        auto setAutoLogic(bool enable) -> void { m_autoLogic = enable; }
        auto consumeAtEndSensor() const -> bool { return m_config.consumeAtEndSensor; }
        auto setConsumeAtEndSensor(bool enable) -> void { m_config.consumeAtEndSensor = enable; }

        auto damperOpen() const -> bool { return m_damperOpen; }
        auto setDamperOpen(bool open) -> void { m_damperOpen = open; }
        auto isInternalMode() const -> bool { return m_internalMode; }
        auto setInternalMode(bool internalMode) -> void { m_internalMode = internalMode; }

      private:
        Config m_config;
        std::shared_ptr<link::ILink> m_link;
        std::vector<Part> m_parts;
        std::vector<bool> m_sensorStates;
        uint32_t m_nextPartId{ 1 };
        bool m_running{ false };    // Simulator thread running
        bool m_beltRunning{ true }; // Physical belt moving
        bool m_autoSpawn{ false };
        bool m_autoLogic{ false }; // Internal sequence logic
        bool m_damperOpen{ false };
        bool m_internalMode{ false };
        double m_autoSpawnTimer{ 0.0 };
        double m_damperTimer{ 0.0 };
        mutable std::mutex m_mutex;
    };
}
