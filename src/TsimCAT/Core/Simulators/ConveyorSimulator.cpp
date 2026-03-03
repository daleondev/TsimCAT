#include "ConveyorSimulator.hpp"
#include "Link/Symbolic/ISymbolicLink.hpp"
#include "Logger/Logger.hpp"
#include "Logger/TraceLogger.hpp"
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
                auto connectResult = co_await client->connect();
                if (!connectResult) {
                    logger::error("{}: Failed to connect symbolic link: {}",
                                  m_config.name,
                                  connectResult.error().message());
                    co_return std::unexpected(connectResult.error());
                }
            }
        }
        co_return result::success();
    }

    auto ConveyorSimulator::start() -> void { m_running = true; }

    auto ConveyorSimulator::stop() -> void { m_running = false; }

    auto ConveyorSimulator::update(double deltaTimeSeconds) -> void
    {
        if (!m_running)
            return;

        std::scoped_lock lock(m_mutex);

        // 1. Auto Spawn Logic
        if (m_autoSpawn && m_beltRunning) {
            m_autoSpawnTimer += deltaTimeSeconds;
            if (m_autoSpawnTimer >= 5.0) { // Spawn every 5 seconds
                m_autoSpawnTimer = 0.0;

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
                    newPart.type = (rand() % 2) + 1;
                    newPart.position = 0;
                    newPart.width = 140;
                    newPart.length = 140;
                    newPart.height = 80;
                    m_parts.push_back(newPart);
                }
            }
        }

        // 2. Sensor Update (Geometric check)
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

        // 3. Autonomous Sequence Logic
        if (m_autoLogic) {
            bool partAtDamper =
              (m_config.damperSensorIndex >= 0 && m_sensorStates[m_config.damperSensorIndex]);
            bool partAtDamperClose =
              (m_config.damperCloseSensorIndex >= 0 &&
               m_config.damperCloseSensorIndex < static_cast<int>(m_sensorStates.size()) &&
               m_sensorStates[m_config.damperCloseSensorIndex]);
            bool partAtEnd = (m_config.endSensorIndex >= 0 && m_sensorStates[m_config.endSensorIndex]);

            if (partAtEnd && m_config.consumeAtEndSensor && m_config.endSensorIndex >= 0 &&
                m_config.endSensorIndex < static_cast<int>(m_config.sensorPositions.size())) {
                const double endSensorPosition = m_config.sensorPositions[m_config.endSensorIndex];
                auto it = std::max_element(m_parts.begin(), m_parts.end(), [](const Part& a, const Part& b) {
                    return a.position < b.position;
                });

                if (it != m_parts.end()) {
                    const double partStart = it->position - it->length / 2;
                    const double partEnd = it->position + it->length / 2;
                    if (endSensorPosition >= partStart && endSensorPosition <= partEnd) {
                        m_parts.erase(it);
                        partAtEnd = false;
                    }
                }
            }

            if (partAtEnd) {
                m_beltRunning = false; // Stop at end, wait for robot pick
            }
            else if (partAtDamper && !m_damperOpen) {
                m_beltRunning = false; // Stop in front of damper
                m_damperTimer += deltaTimeSeconds;
                if (m_damperTimer > m_config.damperOpenDelaySeconds) {
                    m_damperOpen = true;
                    m_damperTimer = 0;
                }
            }
            else {
                // If damper was opened but part moved past it, we could close it,
                // but usually the next part will trigger it.
                // For now: Continue if no part blocking progress.
                m_beltRunning = true;

                const bool closeDamper =
                  m_config.damperCloseSensorIndex >= 0 ? partAtDamperClose : (!partAtDamper);

                if (m_damperOpen && closeDamper) {
                    // Auto-close damper based on configured close sensor (or default behavior)
                    m_damperOpen = false;
                    m_damperTimer = 0;
                }

                if (!partAtDamper) {
                    // Keep timer deterministic for next opening cycle.
                    m_damperTimer = 0;
                }
            }
        }

        // 4. Move parts only if belt is running
        if (m_beltRunning) {
            for (auto& part : m_parts) {
                part.position += m_config.speed * deltaTimeSeconds;
            }
        }

        // 5. Remove parts that fell off the end (Safety cleanup)
        m_parts.erase(
          std::remove_if(m_parts.begin(),
                         m_parts.end(),
                         [this](const Part& p) { return p.position > m_config.length + p.length; }),
          m_parts.end());
    }

    auto ConveyorSimulator::run() -> coro::Task<void>
    {
        auto* symbolic = m_link ? m_link->asSymbolic() : nullptr;
        if (!symbolic)
            co_return;

        while (m_running) {
            if (!m_internalMode && m_link->status() == link::Status::Connected) {
                if (!m_config.adsRunCmd.empty()) {
                    auto runRes = co_await symbolic->read<bool>(m_config.adsRunCmd);
                    if (runRes) {
                        std::scoped_lock lock(m_mutex);
                        // Only override if not in autoLogic mode
                        if (!m_autoLogic)
                            m_beltRunning = *runRes;
                        logger::TraceLogger::instance().emit(
                          logger::TraceCategory::Protocol,
                          m_config.name,
                          "ads_rx_run_cmd",
                          { logger::traceField("symbol", m_config.adsRunCmd),
                            logger::traceField("run", *runRes),
                            logger::traceField("auto_logic", m_autoLogic) });
                    }
                    else {
                        logger::TraceLogger::instance().emit(
                          logger::TraceCategory::Invariant,
                          m_config.name,
                          "ads_rx_run_cmd_failed",
                          { logger::traceField("symbol", m_config.adsRunCmd),
                            logger::traceField("error", runRes.error().message()) });
                    }
                }

                for (size_t i = 0; i < m_config.adsSensorSignals.size(); ++i) {
                    if (i < m_sensorStates.size()) {
                        bool state;
                        {
                            std::scoped_lock lock(m_mutex);
                            state = m_sensorStates[i];
                        }
                        (void)co_await symbolic->write(m_config.adsSensorSignals[i], state);
                        logger::TraceLogger::instance().emit(
                          logger::TraceCategory::Protocol,
                          m_config.name,
                          "ads_tx_sensor",
                          { logger::traceField("index", static_cast<int>(i)),
                            logger::traceField("symbol", m_config.adsSensorSignals[i]),
                            logger::traceField("blocked", state) });
                    }
                }
            }
            else if (!m_internalMode) {
                logger::TraceLogger::instance().emit(
                  logger::TraceCategory::Lifecycle,
                  m_config.name,
                  "ads_disconnected_wait",
                  { logger::traceField("status", static_cast<int>(m_link->status())) });
            }
            co_await coro::sleep(std::chrono::milliseconds(50));
        }
    }

    auto ConveyorSimulator::spawnPart(uint8_t type) -> void
    {
        std::scoped_lock lock(m_mutex);
        for (const auto& part : m_parts) {
            if (part.position < 150.0)
                return;
        }
        Part newPart;
        newPart.id = m_nextPartId++;
        newPart.type = type;
        newPart.position = 0;
        newPart.width = 140;
        newPart.length = 140;
        newPart.height = 80;
        m_parts.push_back(newPart);
    }

    auto ConveyorSimulator::spawnPartAtPosition(uint8_t type, double position) -> void
    {
        std::scoped_lock lock(m_mutex);

        Part newPart;
        newPart.id = m_nextPartId++;
        newPart.type = type;
        newPart.width = 140;
        newPart.length = 140;
        newPart.height = 80;

        const double minPosition = newPart.length * 0.5;
        const double maxPosition = m_config.length - newPart.length * 0.5;
        newPart.position = std::clamp(position, minPosition, maxPosition);

        for (const auto& part : m_parts) {
            if (std::abs(part.position - newPart.position) < (newPart.length * 0.8)) {
                return;
            }
        }

        m_parts.push_back(newPart);
    }

    auto ConveyorSimulator::clearParts() -> void
    {
        std::scoped_lock lock(m_mutex);
        m_parts.clear();
    }

    auto ConveyorSimulator::takePartAtEnd() -> std::optional<Part>
    {
        std::scoped_lock lock(m_mutex);
        if (m_parts.empty())
            return std::nullopt;

        // Find part closest to the end
        auto it = std::max_element(m_parts.begin(), m_parts.end(), [](const Part& a, const Part& b) {
            return a.position < b.position;
        });

        // Check if it's actually near the end (e.g. within 200mm of end sensor)
        if (it->position > m_config.length - 200.0) {
            Part p = *it;
            m_parts.erase(it);
            return p;
        }
        return std::nullopt;
    }

    auto ConveyorSimulator::peekPartAtEnd() const -> std::optional<Part>
    {
        std::scoped_lock lock(m_mutex);
        if (m_parts.empty())
            return std::nullopt;

        auto it = std::max_element(m_parts.begin(), m_parts.end(), [](const Part& a, const Part& b) {
            return a.position < b.position;
        });

        if (it->position > m_config.length - 200.0) {
            return *it;
        }
        return std::nullopt;
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
