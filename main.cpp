#include "AdsLib.h"
#include "AdsNotificationOOI.h"
#include "AdsVariable.h"

#include <print>
#include <atomic>
#include <chrono>
#include <iostream>

static std::atomic_bool s_running{false};

auto main() -> int
{
    std::println("Starting Example");

    try
    {
        bhf::ads::SetLocalAddress({192, 168, 56, 1, 1, 20});
        auto device{AdsDevice(
            "192.168.56.1",
            AmsNetId{192, 168, 56, 1, 1, 1},
            AMSPORT_R0_PLC_TC3)};

        auto state = device.GetState();
        std::cout << "Connected! State: " << state.ads << std::endl;

        // AdsNotificationAttrib attrib{
        //     .cbLength = sizeof(bool),
        //     .nTransMode = ADSTRANS_SERVERCYCLE,
        //     .nMaxDelay = 0,
        //     .nCycleTime = 4000000};

        // auto notification{AdsNotification(
        //     device,
        //     "P_GripperControl.stPneumaticGripperData.bOpen",
        //     attrib,
        //     [](const AmsAddr *pAddr,
        //        const AdsNotificationHeader *pNotification,
        //        uint32_t hUser) -> void
        //     {
        //         std::println("Notified : {}", pNotification->cbSampleSize);
        //     },
        //     0x01)};

        // using namespace std::chrono_literals;
        // s_running = true;
        // while (s_running)
        // {
        // }
        std::cin.get();
        return 0;
    }
    catch (const AdsException &ex)
    {
        std::println("Error: {}", ex.errorCode);
        std::println("AdsException message: {}", ex.what());
        return 1;
    }
    catch (const std::runtime_error &ex)
    {
        std::println("Error: {}", ex.what());
        return 1;
    }
    catch (...)
    {
        std::println("Unknown exception!");
        return 1;
    }
}
