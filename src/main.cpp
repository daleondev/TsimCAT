#include <tlink/tlink.hpp>
#include <tlink/drivers/ads.hpp>
#include <print>

/**
 * TLink Usage Example: Demonstrating the ADS Driver Interface
 */

auto runApp(tlink::Context &ctx) -> tlink::Task<void>
{
    using namespace std::chrono_literals;
    // 1. Initialize the ADS Driver
    tlink::drivers::AdsDriver adsDriver(ctx, "192.168.56.1.1.1", "192.168.56.1", AMSPORT_R0_PLC_TC3, "192.168.56.1.1.20");

    std::println("App: Connecting to PLC via ADS...");
    auto connRes = co_await adsDriver.connect(100ms);

    if (!connRes)
    {
        std::println(stderr, "App: Failed to connect: {}", connRes.error().message());
        ctx.stop();
        co_return;
    }

    std::println("App: Connected!");

    // 2. Perform a one-time Read
    std::println("App: P_GripperControl.stPneumaticGripperData.bOpen...");
    auto readRes = co_await adsDriver.read<bool>("P_GripperControl.stPneumaticGripperData.bOpen");
    if (readRes)
    {
        std::println("Value: {}", readRes.value());
    }

    // 3. Start a Subscription (Asynchronous Stream)
    std::println("App: Subscribing to P_GripperControl.stPneumaticGripperData.bOpen...");
    auto subRes = co_await adsDriver.subscribe("P_GripperControl.stPneumaticGripperData.bOpen", tlink::SubscriptionType::Cyclic, 100ms);

    if (subRes)
    {
        auto sub = subRes.value();
        std::println("App: Subscription active. Waiting for first 3 updates...");

        for (int i = 0; i < 3; ++i)
        {
            // Suspends until the next update is pushed by the driver
            auto update = co_await sub->stream.next();
            if (update)
            {
                std::println("   [Update {:02}] Received {} bytes", i + 1, update.value().size());
            }
        }
    }

    // 4. Cleanup
    std::println("App: Disconnecting...");
    co_await adsDriver.disconnect();
    std::println("App: Shutdown complete.");

    // Stop the event loop
    ctx.stop();
}

auto main() -> int
{
    std::println("TLink Framework: Initialized");
    std::println("---------------------------");

    tlink::Context ctx;

    // Spawn our app coroutine
    auto app = runApp(ctx);
    ctx.schedule(app.m_handle);

    // Start the scheduler loop
    // In a real app, this might be integrated with a UI loop or ASIO context
    ctx.run();

    std::println("---------------------------");
    return 0;
}
