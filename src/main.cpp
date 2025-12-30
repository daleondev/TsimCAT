#include "tlink/drivers/Ads.hpp"
#include "tlink/tlink.hpp"

#include <chrono>
#include <print>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

auto getTimestamp() -> std::string
{
    auto now = std::chrono::system_clock::now();
    return std::format("{:%H:%M:%OS}", now);
}

auto waiterTask(int id, tlink::Subscription<bool> sub) -> tlink::coro::Task<void>
{
    std::println("[{}] Waiter [{}]: Started", getTimestamp(), id);
    try {
        while (true) {
            auto update = co_await sub.stream.next();
            if (update) {
                std::println("[{}] Waiter [{}]: Received Value: {}", getTimestamp(), id, *update);
            }
            else {
                std::println("[{}] Waiter [{}]: Stream closed, exiting.", getTimestamp(), id);
                break;
            }
        }
    } catch (const std::exception& e) {
        std::println(stderr, "[{}] Waiter [{}]: Exception: {}", getTimestamp(), id, e.what());
    }
    co_return;
}

auto runApp(tlink::coro::IExecutor& ex) -> tlink::coro::Task<void>
{
    try {
        tlink::drivers::AdsDriver adsDriver(
          "192.168.56.1.1.1", "192.168.56.1", AMSPORT_R0_PLC_TC3, "192.168.56.1.1.20");

        std::println("[{}] App: Connecting to PLC via ADS...", getTimestamp());
        tlink::Result<void> connRes = co_await adsDriver.connect(100ms);

        if (!connRes) {
            std::println(
              stderr, "[{}] App: Failed to connect: {}", getTimestamp(), connRes.error().message());
            ex.stop();
            co_return;
        }

        std::println("[{}] App: Connected!", getTimestamp());

        std::println("[{}] App: Subscribing to P_GripperControl.stPneumaticGripperData.bOpen...",
                     getTimestamp());
        auto subRes = co_await adsDriver.subscribe<bool>(
          "P_GripperControl.stPneumaticGripperData.bOpen", tlink::SubscriptionType::Cyclic, 500ms);

        if (subRes.has_value()) {
            auto sub = subRes.value();

            // Broadcast is now default!
            // To enable Load Balancer behavior, you would uncomment the following:
            // sub.stream.setMode(tlink::coro::ChannelMode::LoadBalancer);

            std::println("[{}] App: Spawning 3 concurrent waiters on separate threads...", getTimestamp());
            std::vector<std::jthread> threads;
            for (int i = 1; i <= 3; ++i) {
                threads.emplace_back([sub, i] {
                    tlink::coro::Context ctx;
                    co_spawn(ctx, [sub, i, &ctx](auto&) mutable -> tlink::coro::Task<void> {
                        co_await waiterTask(i, sub);
                        ctx.stop();
                    });
                    ctx.run();
                });
            }

            std::println("[{}] App: Main task also joining the stream for 5 updates...", getTimestamp());
            for (int i = 0; i < 5; ++i) {
                auto update = co_await sub.stream.next();
                if (update) {
                    std::println("[{}] App: Main task received update: {}", getTimestamp(), *update);
                }
                else {
                    std::println("[{}] App: Main task stream closed early", getTimestamp());
                    break;
                }
            }

            std::println("[{}] App: Main task done waiting. Unsubscribing now...", getTimestamp());
            // In our RAII model, we can either manually unsubscribe or just let 'sub' go out of scope.
            auto unsubRes = co_await adsDriver.unsubscribe(sub);

            std::this_thread::sleep_for(200ms);

            if (unsubRes) {
                std::println("[{}] App: Unsubscribed successfully!", getTimestamp());
            }
        }
        else {
            std::println(
              stderr, "[{}] App: Subscription failed: {}", getTimestamp(), subRes.error().message());
        }

        std::println("[{}] App: Disconnecting...", getTimestamp());
        co_await adsDriver.disconnect();
        std::println("[{}] App: Shutdown complete.", getTimestamp());

    } catch (const std::exception& e) {
        std::println(stderr, "[{}] App: Exception: {}", getTimestamp(), e.what());
    }

    ex.stop();
}

auto main() -> int
{
    std::println("TLink Framework: Broadcast Validation");
    std::println("------------------------------------");

    try {
        tlink::coro::Context ctx;
        co_spawn(ctx, runApp);
        ctx.run();
    } catch (const std::exception& e) {
        std::println(stderr, "Fatal Exception: {}", e.what());
    }

    std::println("------------------------------------");
    return 0;
}
