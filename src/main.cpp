#include <tlink/tlink.hpp>
#include <print>

/**
 * REPLACED PREVIOUS EXAMPLE WITH TLINK FOUNDATION TEST
 */

// A Mock Driver for testing the interface
class MockDriver : public tlink::IDriver {
public:
    auto connect() -> tlink::Task<tlink::Result<void>> override {
        std::println("MockDriver: Connecting...");
        co_return tlink::success();
    }

    auto disconnect() -> tlink::Task<tlink::Result<void>> override {
        std::println("MockDriver: Disconnecting...");
        co_return tlink::success();
    }

    auto read_raw(std::string_view path) -> tlink::Task<tlink::Result<std::vector<std::byte>>> override {
        std::println("MockDriver: Reading from {}...", path);
        std::vector<std::byte> data{std::byte{0x42}};
        co_return data;
    }

    auto write_raw(std::string_view path, const std::vector<std::byte>& data) -> tlink::Task<tlink::Result<void>> override {
        std::println("MockDriver: Writing {} bytes to {}...", data.size(), path);
        co_return tlink::success();
    }

    auto subscribe(std::string_view path) -> tlink::Task<tlink::Result<std::shared_ptr<tlink::DataStream>>> override {
        std::println("MockDriver: Subscribing to {}...", path);
        
        auto stream = std::make_shared<tlink::DataStream>();
        
        // Simulate an immediate update pushed by the "Driver"
        std::vector<std::byte> data{std::byte{0xAA}, std::byte{0xBB}};
        stream->push(data);
        
        // Simulate another one
        stream->push(data);

        co_return stream;
    }
};

auto run_app(tlink::Context& ctx) -> tlink::Task<void> {
    MockDriver driver;

    auto res = co_await driver.connect();
    if (res) {
        // Test Read
        auto data = co_await driver.read_raw("test/read");
        if (data) {
            std::println("Read Success! First byte: {:02x}", (int)data.value()[0]);
        }

        // Test Subscription (Pull-based!)
        auto sub_res = co_await driver.subscribe("test/sub");
        if (sub_res) {
            auto stream = sub_res.value();
            std::println("Subscription active. Waiting for updates...");

            // Fetch first update
            auto update1 = co_await stream->next();
            if (update1) std::println("Update 1 received: {} bytes", update1.value().size());

            // Fetch second update
            auto update2 = co_await stream->next();
            if (update2) std::println("Update 2 received: {} bytes", update2.value().size());
        }

        co_await driver.disconnect();
    }
}

auto main() -> int
{
    std::println("TLink Framework: Initialized");
    std::println("---------------------------");

    tlink::Context ctx;
    
    // Spawn our test app
    auto app = run_app(ctx);
    ctx.schedule(app.handle);

    // Run the event loop
    ctx.run();

    std::println("---------------------------");
    return 0;
}