#pragma once

#include "tlink/driver.hpp"

#include <AdsLib/AdsLib.h>
#include <AdsLib/AdsNotificationOOI.h>
#include <AdsLib/AdsVariable.h>
#include <AdsLib/AdsDevice.h>

#include <string>
#include <mutex>
#include <unordered_map>

namespace tlink { class Context; }

namespace tlink::drivers
{

    class AdsDriver : public tlink::IDriver
    {
    public:
        /**
         * @brief Construct a new Ads Driver.
         * @param ctx The TLink execution context for scheduling.
         * @param remoteNetId The NetID of the target PLC (e.g. "127.0.0.1.1.1").
         * @param ipAddress The IP address of the target (e.g. "127.0.0.1").
         * @param port The ADS port (default 851 for PLC1).
         */
        AdsDriver(tlink::Context &ctx, std::string_view remoteNetId, std::string ipAddress, uint16_t port = AMSPORT_R0_PLC_TC3, std::string_view localNetId = "");
        ~AdsDriver() override;

        // IDriver Interface Implementation
        auto connect() -> Task<Result<void>> override;
        auto disconnect() -> Task<Result<void>> override;

        auto readInto(std::string_view path, std::span<std::byte> dest) -> Task<Result<size_t>> override;
        auto writeFrom(std::string_view path, std::span<const std::byte> src) -> Task<Result<void>> override;

        auto subscribe(std::string_view path, SubscriptionType type = SubscriptionType::OnChange, std::chrono::milliseconds interval = std::chrono::milliseconds(0)) -> Task<Result<std::shared_ptr<RawSubscription>>> override;
        auto unsubscribe(std::shared_ptr<RawSubscription> subscription) -> Task<Result<void>> override;

    private:
        struct SubscriptionContext
        {
            AdsHandle symbolHandle;
            AdsHandle notificationHandle;
            std::shared_ptr<RawSubscription> stream;

            SubscriptionContext() : symbolHandle(nullptr, {[](uint32_t){return 0;}}), notificationHandle(nullptr, {[](uint32_t){return 0;}}) {}
        };

        static void NotificationCallback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, uint32_t hUser);
        void OnNotification(const AdsNotificationHeader* pNotification);

        tlink::Context &m_ctx;
        AmsNetId m_remoteNetId;
        std::string m_ipAddress;
        uint16_t m_port;
        AmsNetId m_localNetId;

        std::unique_ptr<AdsDevice> m_route;

        std::mutex m_mutex;
        uint32_t m_driverId;
        // Map notification handle ID -> Context
        std::unordered_map<uint32_t, std::shared_ptr<SubscriptionContext>> m_subscriptionContexts;
    };

} // namespace tlink::drivers
