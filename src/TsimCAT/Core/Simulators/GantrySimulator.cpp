#include "GantrySimulator.hpp"
#include "Logger/TraceLogger.hpp"

#include <cmath>

namespace core::sim
{
    GantrySimulator::GantrySimulator(std::shared_ptr<ConveyorSimulator> sourceConveyor,
                                     std::shared_ptr<ConveyorSimulator> targetConveyor,
                                     Config config)
      : m_sourceConveyor(std::move(sourceConveyor))
      , m_targetConveyor(std::move(targetConveyor))
      , m_config(config)
      , m_autoTransfer(config.autoTransfer)
      , m_xPos(config.pickupX)
      , m_zPos(config.zHome)
    {
    }

    auto GantrySimulator::initialize() -> coro::Task<result::Result<void>> { co_return result::success(); }

    auto GantrySimulator::start() -> void { m_running = true; }

    auto GantrySimulator::stop() -> void
    {
        m_running = false;
        m_stage = Stage::Idle;
        m_stageTimer = 0.0;
        m_pendingPickupPart.reset();
        m_carriedPart.reset();
        m_gripperGripped = false;
    }

    auto GantrySimulator::update(double deltaTimeSeconds) -> void
    {
        if (!m_running || !m_autoTransfer || !m_sourceConveyor || !m_targetConveyor) {
            return;
        }

        m_stageTimer += deltaTimeSeconds;

        switch (m_stage) {
            case Stage::Idle: {
                auto part = m_sourceConveyor->peekPartAtEnd();
                if (part.has_value()) {
                    m_pendingPickupPart = std::move(part);
                    m_stage = Stage::MoveToPickup;
                    m_stageTimer = 0.0;
                    logger::TraceLogger::instance().emit(
                      logger::TraceCategory::Flow,
                      "gantry",
                      "pickup_requested",
                      { logger::traceField("part_id", m_pendingPickupPart->id),
                        logger::traceField("part_type", static_cast<int>(m_pendingPickupPart->type)) });
                }
                break;
            }

            case Stage::MoveToPickup:
                if (moveAxis(m_xPos, m_config.pickupX, m_config.xSpeed, deltaTimeSeconds)) {
                    m_stage = Stage::LowerToPickup;
                    m_stageTimer = 0.0;
                }
                break;

            case Stage::LowerToPickup:
                if (moveAxis(m_zPos, m_config.zPickup, m_config.zSpeed, deltaTimeSeconds)) {
                    m_gripperGripped = true;
                    m_stage = Stage::GripAtPickup;
                    m_stageTimer = 0.0;
                }
                break;

            case Stage::GripAtPickup:
                if (m_stageTimer >= m_config.gripSettleSeconds) {
                    if (auto taken = m_sourceConveyor->takePartAtEnd(); taken.has_value()) {
                        m_carriedPart = std::move(taken);
                    }
                    else {
                        m_carriedPart = m_pendingPickupPart;
                    }
                    m_pendingPickupPart.reset();
                    m_stage = Stage::LiftFromPickup;
                    m_stageTimer = 0.0;
                    logger::TraceLogger::instance().emit(
                      logger::TraceCategory::State,
                      "gantry",
                      "part_gripped",
                      { logger::traceField("gripped", m_gripperGripped),
                        logger::traceField("has_part", m_carriedPart.has_value()) });
                }
                break;

            case Stage::LiftFromPickup:
                if (moveAxis(m_zPos, m_config.zHome, m_config.zSpeed, deltaTimeSeconds)) {
                    m_stage = Stage::MoveToDrop;
                    m_stageTimer = 0.0;
                }
                break;

            case Stage::MoveToDrop:
                if (moveAxis(m_xPos, m_config.dropX, m_config.xSpeed, deltaTimeSeconds)) {
                    m_stage = Stage::LowerToDrop;
                    m_stageTimer = 0.0;
                }
                break;

            case Stage::LowerToDrop:
                if (moveAxis(m_zPos, m_config.zDrop, m_config.zSpeed, deltaTimeSeconds)) {
                    m_stage = Stage::ReleaseAtDrop;
                    m_stageTimer = 0.0;
                }
                break;

            case Stage::ReleaseAtDrop:
                if (m_carriedPart.has_value()) {
                    m_targetConveyor->spawnPartAtPosition(m_carriedPart->type, m_config.targetDropPosition);
                    logger::TraceLogger::instance().emit(
                      logger::TraceCategory::Flow,
                      "gantry",
                      "part_dropped",
                      { logger::traceField("part_id", m_carriedPart->id),
                        logger::traceField("part_type", static_cast<int>(m_carriedPart->type)),
                        logger::traceField("drop_pos", m_config.targetDropPosition) });
                }
                m_carriedPart.reset();
                m_gripperGripped = false;
                m_stage = Stage::LiftHome;
                m_stageTimer = 0.0;
                break;

            case Stage::LiftHome:
                if (moveAxis(m_zPos, m_config.zHome, m_config.zSpeed, deltaTimeSeconds)) {
                    m_stage = Stage::Idle;
                    m_stageTimer = 0.0;
                }
                break;
        }
    }

    auto GantrySimulator::xPos() const -> double { return m_xPos; }

    auto GantrySimulator::zPos() const -> double { return m_zPos; }

    auto GantrySimulator::gripperGripped() const -> bool { return m_gripperGripped; }

    auto GantrySimulator::autoTransfer() const -> bool { return m_autoTransfer; }

    auto GantrySimulator::setAutoTransfer(bool enabled) -> void { m_autoTransfer = enabled; }

    auto GantrySimulator::hasCarriedPart() const -> bool { return m_carriedPart.has_value(); }

    auto GantrySimulator::carriedPartType() const -> uint8_t
    {
        return m_carriedPart.has_value() ? m_carriedPart->type : 0;
    }

    auto GantrySimulator::statusText() const -> std::string
    {
        switch (m_stage) {
            case Stage::Idle:
                return "Idle";
            case Stage::MoveToPickup:
                return "Move to pickup";
            case Stage::LowerToPickup:
                return "Lower to pickup";
            case Stage::GripAtPickup:
                return "Grip pickup";
            case Stage::LiftFromPickup:
                return "Lift from pickup";
            case Stage::MoveToDrop:
                return "Move to drop";
            case Stage::LowerToDrop:
                return "Lower to drop";
            case Stage::ReleaseAtDrop:
                return "Release";
            case Stage::LiftHome:
                return "Lift home";
            default:
                return "Unknown";
        }
    }

    auto GantrySimulator::moveAxis(double& value, double target, double speed, double deltaTimeSeconds)
      -> bool
    {
        const double step = speed * deltaTimeSeconds;
        const double diff = target - value;

        if (std::abs(diff) <= step) {
            value = target;
            return true;
        }

        value += (diff > 0.0 ? step : -step);
        return false;
    }
}
