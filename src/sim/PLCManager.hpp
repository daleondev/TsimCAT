#pragma once

#include "ConveyorSimulator.hpp"
#include "RobotSimulator.hpp"
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
        PLCManager(std::shared_ptr<ConveyorSimulator> conveyor, 
                   std::shared_ptr<RobotSimulator> robot)
            : m_conveyor(std::move(conveyor))
            , m_robot(std::move(robot))
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

        /**
         * Forces an immediate write of all simulation states to the PLC.
         * Safe to call from any thread.
         */
        void sync_status()
        {
            if (!m_running || !m_ads) return;
            m_ctx.schedule(tlink::coro::detail::co_spawn_impl(m_ctx, [this](auto&) -> tlink::coro::Task<void> {
                if (m_ads) {
                    (void)co_await m_ads->write("GVL.stConveyorStatus", m_conveyor->get_status());
                    (void)co_await m_ads->write("GVL.stRobotStatus", m_robot->get_status());
                }
            }).getHandle());
        }

    private:
        tlink::coro::Task<void> run_communication()
        {
            m_ads = std::make_unique<tlink::drivers::AdsDriver>(
                "192.168.56.1.1.1", "192.168.56.1", 851, "192.168.56.1.1.20");

            if (auto res = co_await m_ads->connect(); !res) {
                std::println(std::cerr, "PLCManager: ADS Connection failed.");
                co_return;
            }

            std::println("PLCManager: ADS Connection established.");

            // 1. Subscribe to Conveyor control
            auto sub_conv = co_await m_ads->subscribe<model::ConveyorControl>("GVL.stConveyorControl");
            if (sub_conv) {
                tlink::coro::co_spawn(m_ctx, [this, s = sub_conv.value()](auto&) mutable -> tlink::coro::Task<void> {
                    while (m_running) {
                        auto update = co_await s.stream.next();
                        if (update) {
                            m_conveyor->update_control(*update);
                            if (m_ads) (void)co_await m_ads->write("GVL.stConveyorStatus", m_conveyor->get_status());
                        }
                        else break;
                    }
                });
            }

            // 2. Subscribe to Robot control
            auto sub_robot = co_await m_ads->subscribe<model::RobotControl>("GVL.stRobotControl");
            if (sub_robot) {
                tlink::coro::co_spawn(m_ctx, [this, s = sub_robot.value()](auto&) mutable -> tlink::coro::Task<void> {
                    while (m_running) {
                        auto update = co_await s.stream.next();
                        if (update) {
                            m_robot->update_control(*update);
                            if (m_ads) (void)co_await m_ads->write("GVL.stRobotStatus", m_robot->get_status());
                        }
                        else break;
                    }
                });
            }
        }

        std::shared_ptr<ConveyorSimulator> m_conveyor;
        std::shared_ptr<RobotSimulator> m_robot;
        std::unique_ptr<tlink::drivers::AdsDriver> m_ads;
        tlink::coro::Context m_ctx;
        std::jthread m_thread;
        std::atomic<bool> m_running{ false };
    };
}