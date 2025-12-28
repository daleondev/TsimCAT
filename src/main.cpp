#include <tlink/tlink.hpp>
#include <tlink/drivers/ads.hpp>
#include <print>

/**
 * TLink Usage Example: Demonstrating the ADS Driver Interface
 */

auto run_app(tlink::Context& ctx) -> tlink::Task<void> {
    // 1. Initialize the ADS Driver
    // NetID: 127.0.0.1.1.1, IP: 127.0.0.1, Port: 851 (PLC1)
    tlink::drivers::AdsDriver ads_driver("127.0.0.1.1.1", "127.0.0.1", 851);

    std::println("App: Connecting to PLC via ADS...");
    auto conn_res = co_await ads_driver.connect();
    
    if (!conn_res) {
        std::println(stderr, "App: Failed to connect: {}", conn_res.error().message());
        co_return;
    }

    std::println("App: Connected!");

    // 2. Perform a one-time Read
    std::println("App: Reading MAIN.nCounter...");
    auto read_res = co_await ads_driver.read_raw("MAIN.nCounter");
    if (read_res) {
        auto data = read_res.value();
        std::println("App: Read Success! Data size: {} bytes", data.size());
    }

    // 3. Start a Subscription (Asynchronous Stream)
    std::println("App: Subscribing to MAIN.stStatus...");
    auto sub_res = co_await ads_driver.subscribe("MAIN.stStatus");
    
    if (sub_res) {
        auto stream = sub_res.value();
        std::println("App: Subscription active. Waiting for first 3 updates...");

        for (int i = 0; i < 3; ++i) {
            // Suspends until the next update is pushed by the driver
            auto update = co_await stream->next();
            if (update) {
                std::println("   [Update {:02}] Received {} bytes", i + 1, update.value().size());
            }
        }
    }

    // 4. Cleanup
    std::println("App: Disconnecting...");
    co_await ads_driver.disconnect();
    std::println("App: Shutdown complete.");
}

auto main() -> int
{
    std::println("TLink Framework: Initialized");
    std::println("---------------------------");

    tlink::Context ctx;
    
    // Spawn our app coroutine
    auto app = run_app(ctx);
    ctx.schedule(app.handle);

    // Start the scheduler loop
    // In a real app, this might be integrated with a UI loop or ASIO context
    ctx.run();

    std::println("---------------------------");
    return 0;
}
