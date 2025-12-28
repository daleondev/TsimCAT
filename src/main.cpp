#include <tlink/tlink.hpp>
#include <print>

/**
 * REPLACED PREVIOUS EXAMPLE WITH TLINK FOUNDATION TEST
 */

// A Mock Driver for testing the interface
class MockDriver : public tlink::IDriver {
public:
    tlink::Task<tlink::Result<void>> connect() override {
        std::println("MockDriver: Connecting...");
        co_return tlink::success();
    }

    tlink::Task<tlink::Result<void>> disconnect() override {
        std::println("MockDriver: Disconnecting...");
        co_return tlink::success();
    }

    tlink::Task<tlink::Result<std::vector<std::byte>>> read_raw(std::string_view path) override {
        std::println("MockDriver: Reading from {}...", path);
        std::vector<std::byte> data{std::byte{0x42}};
        co_return data;
    }

    tlink::Task<tlink::Result<void>> write_raw(std::string_view path, const std::vector<std::byte>& data) override {
        std::println("MockDriver: Writing {} bytes to {}...", data.size(), path);
        co_return tlink::success();
    }
};

tlink::Task<void> run_app(tlink::Context& ctx) {
    MockDriver driver;

    auto res = co_await driver.connect();
    if (res) {
        auto data = co_await driver.read_raw("test/path");
        if (data) {
            std::println("Read Success! First byte: {:02x}", (int)data.value()[0]);
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