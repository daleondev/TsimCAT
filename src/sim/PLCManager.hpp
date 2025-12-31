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
            
            // 1. Command Subscription Thread
            m_comm_thread = std::jthread([this] {
                tlink::coro::Context ctx;
                tlink::coro::co_spawn(ctx, [this](auto& ex) -> tlink::coro::Task<void> {
                    co_await this->run_subscription_loop();
                });
                ctx.run();
            });

            // 2. Status Update Thread (Persistent connection to avoid "No Dispatcher" warnings)
            m_status_thread = std::jthread([this] {
                this->run_status_loop();
            });
        }

        void stop()
        {
            m_running = false;
        }

    private:
        tlink::coro::Task<void> run_subscription_loop()
        {
            // Use local NetId ... .20
            tlink::drivers::AdsDriver ads(
                "192.168.56.1.1.1", "192.168.56.1", 851, "192.168.56.1.1.20");

            if (auto res = co_await ads.connect(); !res) {
                std::println(std::cerr, "PLCManager (Sub): Failed to connect via ADS");
                co_return;
            }

            std::println("PLCManager: Command connection established.");

            auto sub_res = co_await ads.subscribe<model::ConveyorControl>("GVL.stConveyorControl");
            if (sub_res) {
                auto sub = sub_res.value();
                while (m_running) {
                    auto update = co_await sub.stream.next();
                    if (update) {
                        m_conveyor->update_control(*update);
                    } else {
                        break;
                    }
                }
            } else {
                std::println(std::cerr, "PLCManager: Failed to subscribe to GVL.stConveyorControl");
            }
        }

        void run_status_loop()
        {
            // Use a DIFFERENT local NetId (... .21) for the second persistent connection
            // This is key to avoiding dispatcher conflicts in the ADS library.
            tlink::drivers::AdsDriver ads(
                "192.168.56.1.1.1", "192.168.56.1", 851, "192.168.56.1.1.21");

            tlink::coro::Context ctx;
            tlink::coro::co_spawn(ctx, [this, &ads](auto& ex) -> tlink::coro::Task<void> {
                if (auto res = co_await ads.connect(); !res) {
                    std::println(std::cerr, "PLCManager (Status): Connection failed");
                    co_return;
                }
                
                std::println("PLCManager: Status connection established.");

                while (m_running) {
                    (void)co_await ads.write("GVL.stConveyorStatus", m_conveyor->get_status());
                    // Since this is its own thread, we can safely sleep here
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });
            ctx.run();
        }

        std::shared_ptr<ConveyorSimulator> m_conveyor;
        std::jthread m_comm_thread;
        std::jthread m_status_thread;
        std::atomic<bool> m_running{ false };
    };
}