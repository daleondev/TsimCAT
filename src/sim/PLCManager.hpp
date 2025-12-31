#pragma once

#include "ConveyorSimulator.hpp"
#include "tlink/drivers/Ads.hpp"
#include "tlink/coroutine/coroutine.hpp"
#include <memory>
#include <thread>
#include <print>
#include <iostream>

namespace tsim::sim
{
    class PLCManager
    {
    public:
        PLCManager(std::shared_ptr<ConveyorSimulator> conveyor)
            : m_conveyor(std::move(conveyor))
        {
        }

        ~PLCManager()
        {
            stop();
        }

        void start()
        {
            m_running = true;
            m_comm_thread = std::jthread([this] {
                tlink::coro::co_spawn(m_ctx, [this](auto& ex) -> tlink::coro::Task<void> {
                    co_await this->run_communication();
                });
                m_ctx.run();
            });

            // Status Update Thread (Ensures the PLC sees the final state even if no commands arrive)
            m_status_thread = std::jthread([this] {
                this->run_status_updates();
            });
        }

        void stop()
        {
            m_running = false;
            m_ctx.stop();
        }

    private:
        tlink::coro::Task<void> run_communication()
        {
            tlink::drivers::AdsDriver ads(
                "192.168.56.1.1.1", "192.168.56.1", 851, "192.168.56.1.1.20");

            if (auto res = co_await ads.connect(); !res) {
                std::println(std::cerr, "PLCManager: Failed to connect to PLC via ADS");
                co_return;
            }

            std::println("PLCManager: Connected to PLC");

            // Subscribe to control structure (PLC -> Sim)
            auto sub_res = co_await ads.subscribe<model::ConveyorControl>("GVL.stConveyorControl");
            if (sub_res) {
                auto sub = sub_res.value();
                while (m_running) {
                    auto update = co_await sub.stream.next();
                    if (update) {
                        m_conveyor->update_control(*update);
                        // Also push an immediate update for fast response
                        (void)co_await ads.write("GVL.stConveyorStatus", m_conveyor->get_status());
                    } else {
                        break;
                    }
                }
            } else {
                std::println(std::cerr, "PLCManager: Failed to subscribe to GVL.stConveyorControl");
            }
        }

        void run_status_updates()
        {
            tlink::drivers::AdsDriver ads(
                "192.168.56.1.1.1", "192.168.56.1", 851, "192.168.56.1.1.20");

            tlink::coro::Context local_ctx;
            tlink::coro::co_spawn(local_ctx, [this, &ads](auto&) -> tlink::coro::Task<void> {
                if (auto res = co_await ads.connect(); !res) co_return;

                while (m_running) {
                    (void)co_await ads.write("GVL.stConveyorStatus", m_conveyor->get_status());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Lower freq for background
                }
            });

            while (m_running) {
                local_ctx.run();
            }
        }

        std::shared_ptr<ConveyorSimulator> m_conveyor;
        tlink::coro::Context m_ctx;
        std::jthread m_comm_thread;
        std::jthread m_status_thread;
        std::atomic<bool> m_running{ false };
    };
}
