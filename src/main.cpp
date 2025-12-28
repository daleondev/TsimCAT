#include <generator>
#include <exception>
#include <print>
#include <string_view>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

// -----------------------------------------------------------------------------
// Example Coroutine Logic using C++23 std::generator
// -----------------------------------------------------------------------------

struct SensorData
{
    int id;
    double value;
    std::string_view status;
};

// Simulates a stream of sensor data
std::generator<SensorData> simulate_sensor_stream(int sensor_id, int count)
{
    std::mt19937 gen(std::random_device{}());
    std::normal_distribution<> d(25.0, 2.0); // Mean 25.0, stddev 2.0

    for (int i = 0; i < count; ++i)
    {
        double reading = d(gen);
        std::string_view status = (reading > 28.0) ? "HIGH" : (reading < 22.0 ? "LOW" : "OK");

        // Emulate some processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        co_yield SensorData{sensor_id, reading, status};
    }
}

auto main() -> int
{
    std::println("Starting C++23 Coroutine Example: Sensor Stream Simulation");
    std::println("---------------------------------------------------------");

    try
    {
        // Range-based for loop support via begin()/end() iterators
        int idx = 1;
        for (const auto &data : simulate_sensor_stream(101, 10))
        {
            std::println("Reading #{:02}: Sensor={} Value={:.2f} Status={}",
                         idx++, data.id, data.value, data.status);
        }
    }
    catch (const std::exception &ex)
    {
        std::println(stderr, "Exception caught: {}", ex.what());
        return 1;
    }

    std::println("---------------------------------------------------------");
    std::println("Simulation Complete.");
    return 0;
}