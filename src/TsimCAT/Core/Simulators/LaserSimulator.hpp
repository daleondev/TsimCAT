#pragma once

#include "ISimulator.hpp"
#include "Link/ILink.hpp"
#include <memory>
#include <mutex>
#include <string>

namespace core::sim
{
    class LaserSimulator : public ISimulator
    {
      public:
        enum class State
        {
            Idle,
            Ready,
            Pilot,
            Laser,
            Done,
            Error
        };

        explicit LaserSimulator(std::shared_ptr<link::ILink> link);
        ~LaserSimulator() override;

        auto name() const -> std::string override { return "Laser"; }
        auto initialize() -> coro::Task<result::Result<void>> override;
        auto start() -> void override;
        auto stop() -> void override;
        auto update(double deltaTimeSeconds) -> void override;

        auto run() -> coro::Task<void>;

        // State Access
        auto state() const -> State;
        auto stateString() const -> std::string;
        auto tcpStatus() const -> std::string;
        auto lastMessage() const -> std::string;
        auto setInternalMode(bool internalMode) -> void;
        auto isInternalMode() const -> bool;
        auto startLocalMarking(double durationSeconds = 2.0) -> bool;
        auto acknowledgeDone() -> void;

      private:
        auto handleCommand(std::string_view cmd) -> coro::Task<void>;
        auto sendStatus(std::string_view status) -> coro::Task<void>;
        auto transitionTo(State newState) -> coro::Task<void>;

        std::shared_ptr<link::ILink> m_link;
        std::string m_lastMessage{ "No messages" };
        mutable std::mutex m_mutex;
        bool m_running{ false };
        bool m_internalMode{ false };

        State m_state{ State::Idle };
        double m_workTimer{ 0.0 };
        bool m_laserDonePending{ false };
    };
}

/*
TCP telegrams from PLC to laser:
[get state]     -> asks for laser state (<idle>, <ready>, <pilot>, <laser>, <done>, <error>)
[open shutter]  -> opens shutter            -> <idle> -> <ready>
[close shutter] -> closes shutter           -> <any state (except error)> -> <idle>
[pilot on]      -> turns pilot laser on     -> <ready> -> <pilot>
[pilot off]     -> turns pilot laser off    -> <pilot> -> <ready>
[laser on]      -> turns laser on           -> <ready> -> <laser> -|laser done|-> <done>
[ack done]      -> acknowledges laser done  -> <done> -> <idle>
[ack error]     -> acknowledges error       -> <error> -> <idle>

TCP telegrams from laser to PLC:
[state <state>]     -> returns state (<idle>, <ready>, <pilot>, <laser>, <done>, <error>)
[ready]             -> shutter is open and ready for laser
[pilot busy]        -> pilot laser is on
[laser busy]        -> laser is on
[laser done]        -> laser is done
[error=<code>]      -> error with code is active
*/
