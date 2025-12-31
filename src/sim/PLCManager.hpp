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
            m_thread = std::jthread([this] {
                tlink::coro::co_spawn(m_ctx, [this](auto& ex) -> tlink::coro::Task<void> {
                    co_await this->run_communication();
                });
                m_ctx.run();
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

            // Process incoming control updates (PLC -> Sim)
            auto sub_res = co_await ads.subscribe<model::ConveyorControl>("GVL.stConveyorControl");
            if (sub_res) {
                auto sub = sub_res.value();
                while (m_running) {
                    auto update = co_await sub.stream.next();
                    if (update) {
                        m_conveyor->update_control(*update);
                        
                        // Immediately write back current status after control change
                        auto status = m_conveyor->get_status();
                        (void)co_await ads.write("GVL.stConveyorStatus", status);
                    } else {
                        break;
                    }
                }
            } else {
                std::println(std::cerr, "PLCManager: Failed to subscribe to GVL.stConveyorControl");
            }
        }

        std::shared_ptr<ConveyorSimulator> m_conveyor;
        tlink::coro::Context m_ctx;
        std::jthread m_thread;
        std::atomic<bool> m_running{ false };
    };
}