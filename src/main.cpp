/*=============================================================================
   TsimCAT - The Simulated Control and Automation Toolkit
=============================================================================*/
#include "tlink/log/Logger.hpp"
#include <random>
#include <thread>
#include <vector>

struct SampleData
{
    int id;
    std::string name;
    double value;
};

void worker_thread(int id, int iterations)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delay(1, 10);

    tlink::log::info("Worker {} started", id);

    for (int i = 0; i < iterations; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay(gen)));

        if (i % 10 == 0) {
            tlink::log::warning("Worker {} iteration {}/{} - milestone reached", id, i, iterations);
        }
        else if (i % 3 == 0) {
            SampleData data{ i, "Data-" + std::to_string(i), i * 1.5 };
            tlink::log::debug("Worker {} processing data: {}", id, data);
        }
        else {
            tlink::log::info("Worker {} working... ({})", id, i);
        }
    }

    tlink::log::info("Worker {} finished", id);
}

namespace test
{
    namespace detail
    {
        class IClass
        {
          public:
            virtual const int print3(const int& i) const = 0;
        };

        class MyClass : public IClass
        {
          public:
            void print() { tlink::log::info("Test"); }
            template<typename T>
            bool print2() const
            {
                tlink::log::info("Test");
                return true;
            }
            virtual const int print3(const int& i) const override
            {
                tlink::log::info("Test");
                return i;
            }
            static std::optional<int> print4()
            {
                tlink::log::info("Test");
                return std::nullopt;
            }
            template<typename T>
                requires std::integral<T>
            [[nodiscard]] static inline constexpr auto myFunction(int a, T b = {}) noexcept -> int
            {
                tlink::log::info("Test");
                return a;
            }
            template<typename T>
                requires std::integral<T>
            [[maybe_unused]] inline constexpr const char* myFunction2() const
            {
                tlink::log::info("Test");
                return "";
            }
        };
    }
}

int main(int argc, char* argv[])
{
    tlink::log::LoggerConfig config;
    config.showFile = false;
    config.showLine = false;
    config.showFunction = true;
    tlink::log::Logger::instance().setConfig(config);

    // tlink::log::info("Starting complex multi-threaded logger test...");

    // const int num_threads = 5;
    // const int iterations_per_thread = 50;
    // std::vector<std::thread> threads;

    // tlink::log::info("Spawning {} threads, {} iterations each", num_threads, iterations_per_thread);

    // for (int i = 0; i < num_threads; ++i) {
    //     threads.emplace_back(worker_thread, i, iterations_per_thread);
    // }

    // for (auto& t : threads) {
    //     t.join();
    // }

    // tlink::log::info("All threads joined. Waiting a moment for logs to flush...");
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    test::detail::MyClass c{};
    c.print();
    std::println("1");
    c.print2<const float>();
    c.print3(5);
    test::detail::MyClass::print4();
    std::println("2");
    test::detail::MyClass::myFunction<int>(1);
    c.myFunction2<uint64_t>();

    std::println("3");
    tlink::log::info("Test complete. Exiting.");
    return 0;
}

// #include "tlink/drivers/Ads.hpp"
// #include "tlink/drivers/OpcUa.hpp"
// #include "tlink/tlink.hpp"

// #include <chrono>
// #include <cstring>
// #include <print>
// #include <thread>
// #include <vector>

// using namespace std::chrono_literals;

// auto getTimestamp() -> std::string
// {
//     auto now = std::chrono::system_clock::now();
//     return std::format("{:%H:%M:%OS}", now);
// }

// auto waiterTask(int id, tlink::Subscription<bool> sub) -> tlink::coro::Task<void>
// {
//     std::println("[{}] Waiter [{}]: Started", getTimestamp(), id);
//     try {
//         while (true) {
//             auto update = co_await sub.stream.next();
//             if (update) {
//                 std::println("[{}] Waiter [{}]: Received Value: {}", getTimestamp(), id, *update);
//             }
//             else {
//                 std::println("[{}] Waiter [{}]: Stream closed, exiting.", getTimestamp(), id);
//                 break;
//             }
//         }
//     } catch (const std::exception& e) {
//         std::println(stderr, "[{}] Waiter [{}]: Exception: {}", getTimestamp(), id, e.what());
//     }
//     co_return;
// }

// auto runApp(tlink::coro::IExecutor& ex) -> tlink::coro::Task<void>
// {
//     try {
//         tlink::drivers::AdsDriver adsDriver(
//           "192.168.56.1.1.1", "192.168.56.1", AMSPORT_R0_PLC_TC3, "192.168.56.1.1.20");

//         std::println("[{}] App: Connecting to PLC via ADS...", getTimestamp());
//         tlink::Result<void> connRes = co_await adsDriver.connect(100ms);

//         if (!connRes) {
//             std::println(
//               stderr, "[{}] App: Failed to connect: {}", getTimestamp(), connRes.error().message());
//             ex.stop();
//             co_return;
//         }

//         std::println("[{}] App: Connected!", getTimestamp());

//         std::println("[{}] App: Subscribing to P_GripperControl.stPneumaticGripperData.bOpen...",
//                      getTimestamp());
//         auto subRes = co_await adsDriver.subscribe<bool>(
//           "P_GripperControl.stPneumaticGripperData.bOpen", tlink::SubscriptionType::Cyclic, 500ms);

//         if (subRes.has_value()) {
//             auto sub = subRes.value();

//             // Broadcast is now default!
//             // To enable Load Balancer behavior, you would uncomment the following:
//             // sub.stream.setMode(tlink::coro::ChannelMode::LoadBalancer);

//             std::println("[{}] App: Spawning 3 concurrent waiters on separate threads...", getTimestamp());
//             std::vector<std::jthread> threads;
//             for (int i = 1; i <= 3; ++i) {
//                 threads.emplace_back([sub, i] {
//                     tlink::coro::Context ctx;
//                     co_spawn(ctx, [sub, i, &ctx](auto&) mutable -> tlink::coro::Task<void> {
//                         co_await waiterTask(i, sub);
//                         ctx.stop();
//                     });
//                     ctx.run();
//                 });
//             }

//             std::println("[{}] App: Main task also joining the stream for 5 updates...", getTimestamp());
//             for (int i = 0; i < 5; ++i) {
//                 auto update = co_await sub.stream.next();
//                 if (update) {
//                     std::println("[{}] App: Main task received update: {}", getTimestamp(), *update);
//                 }
//                 else {
//                     std::println("[{}] App: Main task stream closed early", getTimestamp());
//                     break;
//                 }
//             }

//             std::println("[{}] App: Main task done waiting. Unsubscribing now...", getTimestamp());
//             // In our RAII model, we can either manually unsubscribe or just let 'sub' go out of scope.
//             auto unsubRes = co_await adsDriver.unsubscribe(sub);

//             std::this_thread::sleep_for(200ms);

//             if (unsubRes) {
//                 std::println("[{}] App: Unsubscribed successfully!", getTimestamp());
//             }
//         }
//         else {
//             std::println(
//               stderr, "[{}] App: Subscription failed: {}", getTimestamp(), subRes.error().message());
//         }

//         std::println("[{}] App: Disconnecting...", getTimestamp());
//         co_await adsDriver.disconnect();
//         std::println("[{}] App: Shutdown complete.", getTimestamp());

//     } catch (const std::exception& e) {
//         std::println(stderr, "[{}] App: Exception: {}", getTimestamp(), e.what());
//     }

//     ex.stop();
// }

// void testLifetimeSafety()
// {
//     std::println("------------------------------------");
//     std::println("Test: Lifetime Safety (Liveness Token)");

//     // 1. Channel lives OUTSIDE the context
//     tlink::coro::RawBinaryChannel rawChannel;
//     tlink::coro::BinaryChannel<int> channel(rawChannel);

//     {
//         // 2. Context lives INSIDE this scope
//         tlink::coro::Context ctx;
//         std::println("  [Scope] Context Created.");

//         tlink::coro::co_spawn(ctx, [&](auto&) -> tlink::coro::Task<void> {
//             std::println("  [Task] Started. Waiting for data...");
//             // This will register the task with the channel.
//             // It captures the Context's WeakPtr token.
//             auto val = co_await channel.next();

//             // IF the safety fix works, this line should NEVER run because
//             // the channel won't schedule us if the context is dead.
//             if (val) {
//                 std::println(
//                   stderr, "  [Task] CRITICAL ERROR: Resumed after Context destruction! Value: {}", *val);
//                 std::terminate();
//             }
//             else {
//                 std::println("  [Task] Channel closed (expected path if closed explicitly).");
//             }
//         });

//         // Run the context in a background thread so we don't block
//         {
//             std::jthread runner([&] { ctx.run(); });
//             std::this_thread::sleep_for(10ms); // Let task suspend at co_await
//             ctx.stop();
//         } // runner joins here

//         std::println("  [Scope] Context stopped.");

//     } // ctx is destroyed here. The "Life Token" expires.

//     std::println("  [Scope] Context Destroyed.");

//     // 3. Trigger the bug
//     std::println("  [Test] Pushing data to channel (orphan waiter)...");

//     int payload = 42;
//     tlink::coro::RawBinaryChannel::Bytes bytes(sizeof(int));
//     std::memcpy(bytes.data(), &payload, sizeof(int));

//     // If the fix is MISSING, this will crash (access violation on dead executor).
//     // If the fix is PRESENT, this will detect expired token and skip scheduling.
//     rawChannel.push(std::move(bytes));

//     std::println("  [Test] Push complete. No Crash!");
//     std::println("------------------------------------");
// }

// auto cancellationTestTask(tlink::coro::BinaryChannel<int>& chan) -> tlink::coro::Task<void>
// {
//     co_await chan.next();
//     std::println(stderr, "  [Task] CRITICAL ERROR: Cancellation task resumed!");
//     std::terminate();
// }

// void testCancellationSafety()
// {
//     std::println("------------------------------------");
//     std::println("Test: Cancellation Safety");

//     tlink::coro::RawBinaryChannel rawChannel;
//     tlink::coro::BinaryChannel<int> channel(rawChannel);

//     {
//         std::println("  [Scope] Creating task...");
//         auto task = cancellationTestTask(channel);

//         // Manually resume to the first suspension point (the channel.next())
//         // In a real app, co_spawn or awaiting would do this.
//         task.getHandle().resume();

//         std::println("  [Scope] Task is now suspended on channel. Destroying task...");
//     } // task destructor calls m_handle.destroy() -> frame destroyed -> RawAsyncAwaiter destroyed ->
//     removed
//       // from channel

//     std::println("  [Scope] Task destroyed.");

//     std::println("  [Test] Pushing data to channel (should have no waiters)...");
//     int payload = 123;
//     tlink::coro::RawBinaryChannel::Bytes bytes(sizeof(int));
//     std::memcpy(bytes.data(), &payload, sizeof(int));

//     // If unregistration failed, this will attempt to resume a dead handle and crash.
//     rawChannel.push(std::move(bytes));

//     std::println("  [Test] Push complete. No Crash!");
//     std::println("------------------------------------");
// }

// auto runOpcUa(tlink::coro::IExecutor& ex) -> tlink::coro::Task<void>
// {
//     tlink::drivers::UaDriver uaDriver("opc.tcp://127.0.0.1:4840");

//     std::println("[{}] App: Connecting to PLC via OPC UA...", getTimestamp());
//     tlink::Result<void> connRes = co_await uaDriver.connect(100ms);
//     if (!connRes) {
//         std::println(stderr, "[{}] App: Failed to connect: {}", getTimestamp(), connRes.error().message());
//         ex.stop();
//         co_return;
//     }

//     std::println("[{}] App: Connected!", getTimestamp());

//     auto readRes = co_await uaDriver.read<uint32_t>("ns=4;s=GVL.Counter.Counter32", 100ms);
//     if (!readRes) {
//         std::println(stderr, "[{}] App: Failed to read: {}", getTimestamp(), readRes.error().message());
//     }
//     else {
//         std::println("[{}] App: Read value: {}", getTimestamp(), readRes.value());
//     }

//     std::println("[{}] App: Subscribing to ns=4;s=GVL.Counter.Counter32...", getTimestamp());
//     auto subRes = co_await uaDriver.subscribe<uint32_t>(
//       "ns=4;s=GVL.Counter.Counter32", tlink::SubscriptionType::OnChange, 1ms);

//     if (subRes) {
//         auto sub = subRes.value();
//         std::println("[{}] App: Waiting for 5 updates...", getTimestamp());
//         for (int i = 0; i < 5; ++i) {
//             auto update = co_await sub.stream.next();
//             if (update) {
//                 std::println("[{}] App: Received update: {}", getTimestamp(), *update);
//             }
//             else {
//                 std::println("[{}] App: Stream closed", getTimestamp());
//                 break;
//             }
//         }
//         co_await uaDriver.unsubscribe(sub);
//     }
//     else {
//         std::println(stderr, "[{}] App: Subscription failed: {}", getTimestamp(),
//         subRes.error().message());
//     }

//     std::println("[{}] App: Disconnecting...", getTimestamp());
//     co_await uaDriver.disconnect();
//     std::println("[{}] App: Shutdown complete.", getTimestamp());

//     ex.stop();
// }

// auto main() -> int
// {
//     testLifetimeSafety();
//     testCancellationSafety();

//     std::println("TLink Framework: Broadcast Validation");
//     std::println("------------------------------------");

//     try {
//         tlink::coro::Context ctx;
//         co_spawn(ctx, runApp);
//         ctx.run();
//     } catch (const std::exception& e) {
//         std::println(stderr, "Fatal Exception: {}", e.what());
//     }

//     std::println("------------------------------------");
//     return 0;
// }