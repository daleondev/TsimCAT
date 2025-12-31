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
    /**
     * PLCManager manages a single ADS connection to the TwinCAT PLC.
     * It listens for control updates and pushes status changes back to the PLC.
     */
    class PLCManager
    {
    public:
        PLCManager(std::shared_ptr<ConveyorSimulator> conveyor)
            : m_conveyor(std::move(conveyor))
        {
            // Subscribe to simulator events to trigger status writes
            m_conveyor->on_status_change = [this](const model::ConveyorStatus& status) {
                this->request_status_write(status);
            };
        }

        ~PLCManager()
        {
            stop();
        }

        void start()
        {
            m_running = true;
            m_thread = std::jthread([this] {
                // Initialize the coroutine context in this dedicated thread
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
        void request_status_write(const model::ConveyorStatus& status)
        {
            if (!m_running || !m_ads) return;

            // Schedule a one-shot write task in the worker thread's context
            m_ctx.schedule(tlink::coro::detail::co_spawn_impl(m_ctx, [this, status](auto&) -> tlink::coro::Task<void> {
                if (m_ads) {
                    (void)co_await m_ads->write("GVL.stConveyorStatus", status);
                }
            }).getHandle());
        }

        tlink::coro::Task<void> run_communication()
        {
            // ONE single driver instance for the whole process
            m_ads = std::make_unique<tlink::drivers::AdsDriver>(
                "192.168.56.1.1.1", "192.168.56.1", 851, "192.168.56.1.1.20");

            if (auto res = co_await m_ads->connect(); !res) {
                std::cerr << "PLCManager: ADS Connection failed." << std::endl;
                co_return;
            }

            std::cout << "PLCManager: ADS Connection established." << std::endl;

            // Process incoming control updates via subscription
            auto sub_res = co_await m_ads->subscribe<model::ConveyorControl>("GVL.stConveyorControl");
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
                std::cerr << "PLCManager: Failed to subscribe to GVL.stConveyorControl" << std::endl;
            }
        }

        std::shared_ptr<ConveyorSimulator> m_conveyor;
        std::unique_ptr<tlink::drivers::AdsDriver> m_ads;
        tlink::coro::Context m_ctx;
        std::jthread m_thread;
        std::atomic<bool> m_running{ false };
    };
}
