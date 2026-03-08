#pragma once

#include "ISimulator.hpp"

#include "Link/ILink.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace core::sim
{
#pragma pack(push, 1)
    struct RotaryTableControl
    {
        uint8_t bEnable : 1;
        uint8_t bIndex : 1;
        uint8_t bLoadPart : 1;
        uint8_t bReset : 1;
        uint8_t reserved : 4;
        uint8_t nRequestedPartType;
    };

    struct RotaryTableStatus
    {
        float fAngleDeg;
        uint8_t bPartPresent : 1;
        uint8_t bReadyToPick : 1;
        uint8_t bAtLoadPosition : 1;
        uint8_t bAtPickPosition : 1;
        uint8_t bBusy : 1;
        uint8_t reserved : 3;
        uint8_t nPartType;
        uint16_t nIndexPosition;
    };
#pragma pack(pop)

    class RotaryTableSimulator : public ISimulator
    {
      public:
        struct AdsSymbols
        {
            std::string controlSymbol{ "MAIN.stRotaryTableControl" };
            std::string statusSymbol{ "MAIN.stRotaryTableStatus" };
        };

        struct Config
        {
            std::string name{ "RotaryTable" };
            double radius{ 420.0 };
            double height{ 760.0 };
            double loadAngleDeg{ 180.0 };
            double pickAngleDeg{ 0.0 };
            double rotationSpeedDegPerSecond{ 95.0 };
            double loadDelaySeconds{ 1.0 };
        };

        RotaryTableSimulator(Config config, std::shared_ptr<link::ILink> link, AdsSymbols adsSymbols = {});
        ~RotaryTableSimulator() override;

        auto name() const -> std::string override { return m_config.name; }
        auto initialize() -> coro::Task<result::Result<void>> override;
        auto start() -> void override;
        auto stop() -> void override;
        auto update(double deltaTimeSeconds) -> void override;

        auto run() -> coro::Task<void>;

        auto control() const -> RotaryTableControl;
        auto status() const -> RotaryTableStatus;
        auto currentAngleDegrees() const -> double;
        auto partType() const -> uint8_t;
        auto partPresent() const -> bool;
        auto readyToPick() const -> bool;
        auto atLoadPosition() const -> bool;
        auto atPickPosition() const -> bool;
        auto isBusy() const -> bool;

        auto setInternalMode(bool internalMode) -> void;
        auto isInternalMode() const -> bool;
        auto queuePart(uint8_t partType) -> void;
        auto tryLoadPart(uint8_t partType) -> bool;
        auto takePartForRobot() -> uint8_t;

      private:
        auto updateStatusLocked() -> void;

        Config m_config;
        std::shared_ptr<link::ILink> m_link;
        AdsSymbols m_adsSymbols;

        mutable std::mutex m_mutex;
        std::atomic<bool> m_running{ false };
        bool m_internalMode{ false };
        RotaryTableControl m_control{};
        RotaryTableStatus m_status{};
        double m_currentAngleDeg{ 0.0 };
        double m_targetAngleDeg{ 0.0 };
        double m_loadTimerSeconds{ 0.0 };
        bool m_hasPart{ false };
        uint8_t m_partType{ 0 };
    };
}