#include "tlink/drivers/Ads.hpp"
#include "tlink/tlink.hpp"

#include <print>

using namespace std::chrono_literals;

auto waiterTask(int id, tlink::Subscription<bool> sub) -> tlink::coro::Task<void>
{
    std::println("Waiter [{}]: Started", id);
    try {
        while (true) {
            auto update = co_await sub.stream.next();
            if (update) {
                std::println("Waiter [{}]: Received Value: {}", id, *update);
            } else {
                std::println("Waiter [{}]: Stream closed, exiting.", id);
                break;
            }
        }
    } catch (const std::exception& e) {
        std::println(stderr, "Waiter [{}]: Exception: {}", id, e.what());
    }
    co_return;
}

auto runApp(tlink::coro::IExecutor& ex) -> tlink::coro::Task<void>
{
    try {
        tlink::drivers::AdsDriver adsDriver(
          ex, "192.168.56.1.1.1", "192.168.56.1", AMSPORT_R0_PLC_TC3, "192.168.56.1.1.20");

        std::println("App: Connecting to PLC via ADS...");
        tlink::Result<void> connRes = co_await adsDriver.connect(100ms);

        if (!connRes) {
            std::println(stderr, "App: Failed to connect: {}", connRes.error().message());
            ex.stop();
            co_return;
        }

        std::println("App: Connected!");

        std::println("App: Subscribing to P_GripperControl.stPneumaticGripperData.bOpen...");
        auto subRes = co_await adsDriver.subscribe<bool>(
          "P_GripperControl.stPneumaticGripperData.bOpen", tlink::SubscriptionType::Cyclic, 500ms);

        if (subRes.has_value()) {
            auto sub = subRes.value();
            
            std::println("App: Spawning 3 concurrent waiters on the same stream...");
            co_spawn(ex, [sub](auto&) mutable { return waiterTask(1, sub); });
            co_spawn(ex, [sub](auto&) mutable { return waiterTask(2, sub); });
            co_spawn(ex, [sub](auto&) mutable { return waiterTask(3, sub); });

            std::println("App: Main task also joining the stream for 5 updates...");
            for (int i = 0; i < 5; ++i) {
                 auto update = co_await sub.stream.next();
                 if (update) {
                     std::println("App: Main task received update: {}", *update);
                 } else {
                     std::println("App: Main task stream closed early");
                     break;
                 }
            }

            std::println("App: Main task done waiting. Unsubscribing now...");
            auto unsubRes = co_await adsDriver.unsubscribe(sub);
            
            std::this_thread::sleep_for(200ms);
            
            if (unsubRes) {
                std::println("App: Unsubscribed successfully!");
            }
        } else {
            std::println(stderr, "App: Subscription failed: {}", subRes.error().message());
        }

        std::println("App: Disconnecting...");
        co_await adsDriver.disconnect();
        std::println("App: Shutdown complete.");

    } catch (const std::exception& e) {
        std::println(stderr, "App: Exception: {}", e.what());
    }

    ex.stop();
}

auto main() -> int
{
    std::println("TLink Framework: Validation Test");
    std::println("-------------------------------");

    try {
        tlink::coro::Context ctx;
        co_spawn(ctx, runApp);
        ctx.run();
    } catch (const std::exception& e) {
        std::println(stderr, "Fatal Exception: {}", e.what());
    }

    std::println("-------------------------------");
    return 0;
}
