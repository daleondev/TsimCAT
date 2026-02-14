#pragma once

#include "ISimulator.hpp"
#include "Part.hpp"
#include "Link/ILink.hpp"
#include <vector>
#include <mutex>
#include <memory>
#include <string>

namespace core::sim
{
    class ConveyorSimulator : public ISimulator
    {
      public:
        struct Config
        {
            std::string name;
            double length;      // mm
            double speed;       // mm/s
            std::vector<double> sensorPositions; // mm from start
            
            // Logic config
            int damperSensorIndex{ -1 }; // Index in sensorPositions where damper is
            int endSensorIndex{ -1 };    // Index in sensorPositions where end is

            // ADS Mapping
            std::string adsRunCmd;     // e.g., "MAIN.bEntryRun"
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
        auto clearParts() -> void;
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

        auto damperOpen() const -> bool { return m_damperOpen; }
        auto setDamperOpen(bool open) -> void { m_damperOpen = open; }

      private:
        Config m_config;
        std::shared_ptr<link::ILink> m_link;
        std::vector<Part> m_parts;
        std::vector<bool> m_sensorStates;
        uint32_t m_nextPartId{ 1 };
        bool m_running{ false };     // Simulator thread running
        bool m_beltRunning{ true };  // Physical belt moving
        bool m_autoSpawn{ false };
        bool m_autoLogic{ false };   // Internal sequence logic
        bool m_damperOpen{ false };
        double m_autoSpawnTimer{ 0.0 };
        double m_damperTimer{ 0.0 };
        mutable std::mutex m_mutex;
    };
}
