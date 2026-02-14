#pragma once

#include "ConveyorSimulator.hpp"
#include "ISimulator.hpp"
#include "Part.hpp"

#include <memory>
#include <optional>
#include <string>

namespace core::sim
{
    class GantrySimulator : public ISimulator
    {
      public:
        struct Config
        {
            double pickupX{ -450.0 };
            double dropX{ 450.0 };
            double zHome{ 220.0 };
            double zPickup{ 20.0 };
            double zDrop{ 20.0 };
            double xSpeed{ 500.0 };
            double zSpeed{ 350.0 };
            bool autoTransfer{ true };
        };

        GantrySimulator(std::shared_ptr<ConveyorSimulator> sourceConveyor,
                        std::shared_ptr<ConveyorSimulator> targetConveyor,
                        Config config);

        auto name() const -> std::string override { return "Gantry"; }
        auto initialize() -> coro::Task<result::Result<void>> override;
        auto start() -> void override;
        auto stop() -> void override;
        auto update(double deltaTimeSeconds) -> void override;

        auto xPos() const -> double;
        auto zPos() const -> double;
        auto gripperGripped() const -> bool;
        auto autoTransfer() const -> bool;
        auto setAutoTransfer(bool enabled) -> void;
        auto hasCarriedPart() const -> bool;
        auto carriedPartType() const -> uint8_t;
        auto statusText() const -> std::string;

      private:
        enum class Stage
        {
            Idle,
            MoveToPickup,
            LowerToPickup,
            LiftFromPickup,
            MoveToDrop,
            LowerToDrop,
            ReleaseAtDrop,
            LiftHome
        };

        auto moveAxis(double& value, double target, double speed, double deltaTimeSeconds) -> bool;

        std::shared_ptr<ConveyorSimulator> m_sourceConveyor;
        std::shared_ptr<ConveyorSimulator> m_targetConveyor;
        Config m_config;

        bool m_running{ false };
        bool m_autoTransfer{ true };
        Stage m_stage{ Stage::Idle };
        double m_xPos{ 0.0 };
        double m_zPos{ 0.0 };
        bool m_gripperGripped{ false };
        std::optional<Part> m_carriedPart;
    };
}
