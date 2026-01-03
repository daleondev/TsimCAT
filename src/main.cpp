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
        class MyClass
        {
          public:
            void print() { tlink::log::info("Test"); }
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

    tlink::log::info("Test complete. Exiting.");
    return 0;
}