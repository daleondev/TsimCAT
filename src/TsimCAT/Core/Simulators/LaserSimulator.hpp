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
        explicit LaserSimulator(std::shared_ptr<link::ILink> link);
        ~LaserSimulator() override;

        auto name() const -> std::string override { return "Laser"; }
        auto initialize() -> coro::Task<void> override;
        auto start() -> void override;
        auto stop() -> void override;
        auto update(double deltaTimeSeconds) -> void override;

        auto run() -> coro::Task<void>;

        // State Access
        auto tcpStatus() const -> std::string;
        auto lastMessage() const -> std::string;

      private:
        auto runLoop() -> coro::Task<void>;

        std::shared_ptr<link::ILink> m_link;
        std::string m_lastMessage{ "No messages" };
        mutable std::mutex m_mutex;
        bool m_running{ false };
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
