#include "ConveyorSimulator.hpp"
#include "Link/Symbolic/ISymbolicLink.hpp"
#include "Logger/Logger.hpp"
#include <algorithm>

namespace core::sim
{
    ConveyorSimulator::ConveyorSimulator(Config config, std::shared_ptr<link::ILink> link)
      : m_config(std::move(config))
      , m_link(std::move(link))
    {
        m_sensorStates.resize(m_config.sensorPositions.size(), false);
    }

    ConveyorSimulator::~ConveyorSimulator() = default;

    auto ConveyorSimulator::initialize() -> coro::Task<result::Result<void>>
    {
        if (m_link) {
            if (auto* client = m_link->asClient()) {
                co_await client->connect();
            }
        }
        co_return result::success();
    }

    auto ConveyorSimulator::start() -> void
    {
        m_running = true;
    }

    auto ConveyorSimulator::stop() -> void
    {
        m_running = false;
    }

    auto ConveyorSimulator::update(double deltaTimeSeconds) -> void
    {
        if (!m_running) return;

        std::scoped_lock lock(m_mutex);

        // 1. Auto Spawn Logic
        if (m_autoSpawn && m_beltRunning) {
            m_autoSpawnTimer += deltaTimeSeconds;
            if (m_autoSpawnTimer >= 5.0) { // Spawn every 5 seconds
                m_autoSpawnTimer = 0.0;
                
                // Check if start is clear (local logic within update already has lock)
                bool clear = true;
                for (const auto& part : m_parts) {
                    if (part.position < 200.0) { 
                        clear = false;
                        break;
                    }
                }
                
                if (clear) {
                    Part newPart;
                    newPart.id = m_nextPartId++;
                    newPart.type = (rand() % 2) + 1; // Randomly 1 or 2
                    newPart.position = 0;
                    newPart.width = 100;
                    newPart.length = 100;
                    newPart.height = 50;
                    m_parts.push_back(newPart);
                }
            }
        }

        // 2. Move parts only if belt is running
        if (m_beltRunning) {
            for (auto& part : m_parts) {
                part.position += m_config.speed * deltaTimeSeconds;
            }
        }

        // 2. Remove parts that fell off the end
        m_parts.erase(
            std::remove_if(m_parts.begin(), m_parts.end(), 
                [this](const Part& p) { return p.position > m_config.length + p.length; }), 
            m_parts.end());

        // 3. Update sensor states
        for (size_t i = 0; i < m_config.sensorPositions.size(); ++i) {
            double sensorPos = m_config.sensorPositions[i];
            bool blocked = false;
            for (const auto& part : m_parts) {
                double partStart = part.position - part.length / 2;
                double partEnd = part.position + part.length / 2;
                if (sensorPos >= partStart && sensorPos <= partEnd) {
                    blocked = true;
                    break;
                }
            }
            m_sensorStates[i] = blocked;
        }
    }

    auto ConveyorSimulator::run() -> coro::Task<void>
    {
        auto* symbolic = m_link ? m_link->asSymbolic() : nullptr;
        if (!symbolic) co_return;

        while (m_running) {
            if (m_link->status() == link::Status::Connected) {
                // 1. Read Belt Run Command
                if (!m_config.adsRunCmd.empty()) {
                    auto runRes = co_await symbolic->read<bool>(m_config.adsRunCmd);
                    if (runRes) {
                        std::scoped_lock lock(m_mutex);
                        m_beltRunning = *runRes;
                    }
                }

                // 2. Write Sensor States
                for (size_t i = 0; i < m_config.adsSensorSignals.size(); ++i) {
                    if (i < m_sensorStates.size()) {
                        bool state;
                        {
                            std::scoped_lock lock(m_mutex);
                            state = m_sensorStates[i];
                        }
                        (void)co_await symbolic->write(m_config.adsSensorSignals[i], state);
                    }
                }
            }
            co_await coro::sleep(std::chrono::milliseconds(50));
        }
    }

    auto ConveyorSimulator::spawnPart(uint8_t type) -> void
    {
        std::scoped_lock lock(m_mutex);
        
        // Spawn at position 0 (centered at start)
        // Check if start is clear
        for (const auto& part : m_parts) {
            if (part.position < 150.0) { // Assume 150mm clearance needed
                return; 
            }
        }

        Part newPart;
        newPart.id = m_nextPartId++;
        newPart.type = type;
        newPart.position = 0;
        newPart.width = 100;
        newPart.length = 100;
        newPart.height = 50;
        
        m_parts.push_back(newPart);
    }

    auto ConveyorSimulator::clearParts() -> void
    {
        std::scoped_lock lock(m_mutex);
        m_parts.clear();
    }

    auto ConveyorSimulator::parts() const -> std::vector<Part>
    {
        std::scoped_lock lock(m_mutex);
        return m_parts;
    }

    auto ConveyorSimulator::sensors() const -> std::vector<bool>
    {
        std::scoped_lock lock(m_mutex);
        return m_sensorStates;
    }
}
