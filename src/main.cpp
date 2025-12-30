#include "tlink/drivers/Ads.hpp"
#include "tlink/tlink.hpp"

#include <print>

auto runApp(tlink::coro::IExecutor& ex) -> tlink::coro::Task<void>
{
    using namespace std::chrono_literals;

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

    std::println("App: P_GripperControl.stPneumaticGripperData.bOpen...");
    tlink::Result<bool> readRes =
      co_await adsDriver.read<bool>("P_GripperControl.stPneumaticGripperData.bOpen");
    if (readRes) {
        std::println("Value: {}", readRes.value());
    }

    // 3. Start a Subscription (Asynchronous Stream)
    std::println("App: Subscribing to P_GripperControl.stPneumaticGripperData.bOpen...");
    auto subRes = co_await adsDriver.subscribeRaw(
      "P_GripperControl.stPneumaticGripperData.bOpen", tlink::SubscriptionType::Cyclic, 1000ms);

    if (subRes) {
        auto sub = subRes.value();
        std::println("App: Subscription active. Waiting for first 3 updates...");

        for (int i = 0; i < 3; ++i) {
            // Suspends until the next update is pushed by the driver
            auto update = co_await sub->stream.next();
            // if (update) {
            std::println("   [Update {:02}] Received {} bytes", i + 1, update.size());
            // }
        }

        std::println("App: Unsubscribing to P_GripperControl.stPneumaticGripperData.bOpen...");
        auto unsubRes = co_await adsDriver.unsubscribe(sub);
        if (unsubRes) {
            std::println("App: Unsubscribed!");
        }
    }

    std::println("App: Disconnecting...");
    co_await adsDriver.disconnect();
    std::println("App: Shutdown complete.");

    ex.stop();
}

auto main() -> int
{
    std::println("TLink Framework: Initialized");
    std::println("---------------------------");

    tlink::coro::Context ctx;
    co_spawn(ctx, runApp);
    ctx.run();

    std::println("---------------------------");
    return 0;
}
