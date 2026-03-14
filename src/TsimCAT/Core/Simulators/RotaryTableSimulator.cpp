#include "RotaryTableSimulator.hpp"

#include "Link/Symbolic/ISymbolicLink.hpp"
#include "Link/Symbolic/LocalAdsLink.hpp"
#include "Logger/Logger.hpp"
#include "Logger/TraceLogger.hpp"

#include <cmath>

namespace core::sim
{
    namespace
    {
        constexpr double angleToleranceDegrees = 1.0;
    }

    RotaryTableSimulator::RotaryTableSimulator(Config config,
                                               std::shared_ptr<link::ILink> link,
                                               AdsSymbols adsSymbols)
      : m_config(std::move(config))
      , m_link(std::move(link))
      , m_adsSymbols(std::move(adsSymbols))
      , m_currentAngleDeg(m_config.loadAngleDeg)
      , m_targetAngleDeg(m_config.loadAngleDeg)
    {
        updateStatusLocked();
    }

    RotaryTableSimulator::~RotaryTableSimulator() { stop(); }

    auto RotaryTableSimulator::initialize() -> coro::Task<result::Result<void>>
    {
        if (!m_link || m_internalMode || dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get())) {
            co_return result::success();
        }

        if (auto* client = m_link->asClient()) {
            co_return co_await client->connect();
        }

        co_return result::success();
    }

    auto RotaryTableSimulator::start() -> void { m_running = true; }

    auto RotaryTableSimulator::stop() -> void { m_running = false; }

    auto RotaryTableSimulator::update(double deltaTimeSeconds) -> void
    {
        if (!m_running) {
            return;
        }

        if (!m_internalMode) {
            if (auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get())) {
                std::scoped_lock lock(m_mutex);
                m_control = localAds->readSync<RotaryTableControl>(m_adsSymbols.controlSymbol);
            }
        }

        RotaryTableStatus statusCopy{};
        {
            std::scoped_lock lock(m_mutex);

            if (m_control.bReset) {
                m_hasPart = false;
                m_currentAngleDeg = m_config.loadAngleDeg;
                m_targetAngleDeg = m_config.loadAngleDeg;
                m_loadTimerSeconds = 0.0;
            }

            if (m_control.bEnable) {
                const bool atLoad =
                  std::abs(m_currentAngleDeg - m_config.loadAngleDeg) <= angleToleranceDegrees;
                const bool atPick =
                  std::abs(m_currentAngleDeg - m_config.pickAngleDeg) <= angleToleranceDegrees;

                if (m_control.bLoadPart && !m_hasPart && atLoad) {
                    m_loadTimerSeconds += deltaTimeSeconds;
                    if (m_loadTimerSeconds >= m_config.loadDelaySeconds) {
                        m_hasPart = true;
                        m_loadTimerSeconds = 0.0;
                        logger::TraceLogger::instance().emit(
                          logger::TraceCategory::State,
                          "rotary_table",
                          "part_loaded",
                          { logger::traceField("angle", m_currentAngleDeg) });
                    }
                }
                else {
                    m_loadTimerSeconds = 0.0;
                }

                if (m_control.bIndex && m_hasPart && atLoad) {
                    m_targetAngleDeg = m_config.pickAngleDeg;
                    logger::TraceLogger::instance().emit(
                      logger::TraceCategory::State,
                      "rotary_table",
                      "index_started",
                      { logger::traceField("target_angle", m_config.pickAngleDeg) });
                }
            }

            const double diff = m_targetAngleDeg - m_currentAngleDeg;
            const double step = m_config.rotationSpeedDegPerSecond * deltaTimeSeconds;
            if (std::abs(diff) > step) {
                m_currentAngleDeg += std::copysign(step, diff);
            }
            else {
                m_currentAngleDeg = m_targetAngleDeg;
            }

            updateStatusLocked();
            statusCopy = m_status;
        }

        if (!m_internalMode) {
            if (auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get())) {
                localAds->writeSync(m_adsSymbols.statusSymbol, statusCopy);
            }
        }
    }

    auto RotaryTableSimulator::run() -> coro::Task<void>
    {
        if (m_internalMode || !m_link || dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get())) {
            co_return;
        }

        auto* symbolic = m_link->asSymbolic();
        if (!symbolic) {
            co_return;
        }

        while (m_running) {
            if (m_link->status() == link::Status::Connected) {
                auto control = co_await symbolic->read<RotaryTableControl>(m_adsSymbols.controlSymbol);
                if (control) {
                    std::scoped_lock lock(m_mutex);
                    m_control = *control;
                    logger::TraceLogger::instance().emit(
                      logger::TraceCategory::Protocol,
                      "rotary_table",
                      "ads_rx_control",
                      { logger::traceField("enable", m_control.bEnable != 0),
                        logger::traceField("index", m_control.bIndex != 0),
                        logger::traceField("load", m_control.bLoadPart != 0) });
                }

                RotaryTableStatus statusCopy{};
                {
                    std::scoped_lock lock(m_mutex);
                    statusCopy = m_status;
                }
                (void)co_await symbolic->write(m_adsSymbols.statusSymbol, statusCopy);
                logger::TraceLogger::instance().emit(
                  logger::TraceCategory::Protocol,
                  "rotary_table",
                  "ads_tx_status",
                  { logger::traceField("angle", static_cast<double>(statusCopy.fAngleDeg)),
                    logger::traceField("part_present", statusCopy.bPartPresent != 0),
                    logger::traceField("ready_to_pick", statusCopy.bReadyToPick != 0),
                    logger::traceField("busy", statusCopy.bBusy != 0) });
            }

            co_await coro::sleep(std::chrono::milliseconds(50));
        }
    }

    auto RotaryTableSimulator::control() const -> RotaryTableControl
    {
        std::scoped_lock lock(m_mutex);
        return m_control;
    }

    auto RotaryTableSimulator::status() const -> RotaryTableStatus
    {
        std::scoped_lock lock(m_mutex);
        return m_status;
    }

    auto RotaryTableSimulator::currentAngleDegrees() const -> double
    {
        std::scoped_lock lock(m_mutex);
        return m_currentAngleDeg;
    }

    auto RotaryTableSimulator::partPresent() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_hasPart;
    }

    auto RotaryTableSimulator::readyToPick() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_status.bReadyToPick != 0;
    }

    auto RotaryTableSimulator::atLoadPosition() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_status.bAtLoadPosition != 0;
    }

    auto RotaryTableSimulator::atPickPosition() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_status.bAtPickPosition != 0;
    }

    auto RotaryTableSimulator::isBusy() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_status.bBusy != 0;
    }

    auto RotaryTableSimulator::setInternalMode(bool internalMode) -> void
    {
        std::scoped_lock lock(m_mutex);
        m_internalMode = internalMode;
    }

    auto RotaryTableSimulator::isInternalMode() const -> bool
    {
        std::scoped_lock lock(m_mutex);
        return m_internalMode;
    }

    auto RotaryTableSimulator::queuePart() -> void
    {
        if (auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get())) {
            RotaryTableControl control{};
            control.bEnable = 1;
            control.bLoadPart = 1;
            localAds->writeSync(m_adsSymbols.controlSymbol, control);
            return;
        }

        std::scoped_lock lock(m_mutex);
        if (!m_internalMode || m_hasPart) {
            return;
        }

        m_control.bEnable = 1;
        m_control.bLoadPart = 1;
        m_control.bIndex = 0;
    }

    auto RotaryTableSimulator::tryLoadPart() -> bool
    {
        RotaryTableStatus statusCopy{};
        {
            std::scoped_lock lock(m_mutex);
            const bool atLoad = std::abs(m_currentAngleDeg - m_config.loadAngleDeg) <= angleToleranceDegrees;
            if (m_hasPart || !atLoad) {
                return false;
            }

            m_hasPart = true;
            m_loadTimerSeconds = 0.0;
            updateStatusLocked();
            statusCopy = m_status;
        }

        if (auto* localAds = dynamic_cast<link::symbolic::LocalAdsLink*>(m_link.get())) {
            localAds->writeSync(m_adsSymbols.statusSymbol, statusCopy);
        }

        return true;
    }

    auto RotaryTableSimulator::takePartForRobot() -> bool
    {
        std::scoped_lock lock(m_mutex);
        if (!m_hasPart || m_status.bReadyToPick == 0) {
            return false;
        }

        m_hasPart = false;
        m_targetAngleDeg = m_config.loadAngleDeg;
        updateStatusLocked();
        logger::TraceLogger::instance().emit(logger::TraceCategory::State,
                                             "rotary_table",
                                             "part_taken_by_robot",
                                             { logger::traceField("return_angle", m_config.loadAngleDeg) });
        return true;
    }

    auto RotaryTableSimulator::updateStatusLocked() -> void
    {
        const bool atLoad = std::abs(m_currentAngleDeg - m_config.loadAngleDeg) <= angleToleranceDegrees;
        const bool atPick = std::abs(m_currentAngleDeg - m_config.pickAngleDeg) <= angleToleranceDegrees;

        m_status.fAngleDeg = static_cast<float>(m_currentAngleDeg);
        m_status.bPartPresent = m_hasPart ? 1 : 0;
        m_status.bReadyToPick = (m_hasPart && atPick) ? 1 : 0;
        m_status.bAtLoadPosition = atLoad ? 1 : 0;
        m_status.bAtPickPosition = atPick ? 1 : 0;
        m_status.bBusy = std::abs(m_currentAngleDeg - m_targetAngleDeg) > angleToleranceDegrees ? 1 : 0;
        m_status.nIndexPosition = atPick ? 1 : 0;
    }
}