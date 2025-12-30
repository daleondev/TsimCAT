#include "tlink/drivers/Ads.hpp"
#include "tlink/tlink.hpp"

#include <print>

using namespace std::chrono_literals;

auto waiterTask(int id, tlink::Subscription<bool>& sub) -> tlink::coro::Task<void>
{
    std::println("Waiter [{}]: Started", id);
    while (true) {
        auto update = co_await sub.stream.next();
        if (update) {
            std::println("Waiter [{}]: Received Value: {}", id, *update);
        }
        else {
            std::println("Waiter [{}]: Stream closed, exiting.", id);
            break;
        }
    }
}

auto runApp(tlink::coro::IExecutor& ex) -> tlink::coro::Task<void>
{
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

    if (subRes) {
        auto sub = subRes.value();

                std::println("App: Spawning 3 concurrent waiters on the same stream...");
                co_spawn(ex, [&sub](auto&) { return waiterTask(1, sub); });
                co_spawn(ex, [&sub](auto&) { return waiterTask(2, sub); });
                co_spawn(ex, [&sub](auto&) { return waiterTask(3, sub); });
        
                // Instead of blocking the thread, we wait for 10 updates to pass through the waiters.
                // We can do this by just suspending the main task for a while if we had a timer,
                // but since we don't, we'll have the main task also take 5 updates.
                // This validates that 4 waiters (1, 2, 3 + main) can share the stream.
                std::println("App: Main task also joining the stream for 5 updates...");
                for (int i = 0; i < 5; ++i) {
                     auto update = co_await sub.stream.next();
                     if (update) {
                         std::println("App: Main task received update: {}", *update);
                     }
                }
        
                std::println("App: Main task done waiting. Unsubscribing now...");
                auto unsubRes = co_await adsDriver.unsubscribe(sub);
        // Give waiters a moment to print their exit messages
        std::this_thread::sleep_for(200ms);

        if (unsubRes) {
            std::println("App: Unsubscribed successfully!");
        }
    }

    std::println("App: Disconnecting...");
    co_await adsDriver.disconnect();
    std::println("App: Shutdown complete.");

    ex.stop();
}

auto main() -> int
{
    std::println("TLink Framework: Validation Test");
    std::println("-------------------------------");

    tlink::coro::Context ctx;
    co_spawn(ctx, runApp);
    ctx.run();

    std::println("-------------------------------");
    return 0;
}
