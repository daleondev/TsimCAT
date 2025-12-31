#pragma once

#include "tlink/Driver.hpp"

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace tlink::drivers
{
    class UaDriver : public tlink::IDriver
    {
      public:
        explicit UaDriver(std::string endpointUrl);
        ~UaDriver() override;

        // clang-format off
        auto connect(std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> override;
        auto disconnect(std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> override;

        auto readInto(std::string_view path,
                      std::span<std::byte> dest,
                      std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<size_t>> override;
        auto writeFrom(std::string_view path,
                       std::span<const std::byte> src,
                       std::chrono::milliseconds timeout = NO_TIMEOUT) -> coro::Task<Result<void>> override;

        auto subscribeRaw(std::string_view path,
                       size_t size,
                       SubscriptionType type = SubscriptionType::OnChange,
                       std::chrono::milliseconds interval = NO_TIMEOUT) -> coro::Task<Result<std::shared_ptr<RawSubscription>>> override;
        auto unsubscribeRaw(std::shared_ptr<RawSubscription> subscription) -> coro::Task<Result<void>> override;
        auto unsubscribeRawSync(uint64_t id) -> void override;
        // clang-format on

      private:
        auto handleChannelState(UA_SecureChannelState state) -> void;
        auto handleSessionState(UA_SessionState state) -> void;
        auto worker() -> void;

        static void dataChangeNotificationCallback(UA_Client* client,
                                                   UA_UInt32 subId,
                                                   void* subContext,
                                                   UA_UInt32 monId,
                                                   void* monContext,
                                                   UA_DataValue* value);
        void handleDataChange(UA_UInt32 monId, UA_DataValue* value);

        std::string m_endpointUrl;
        bool m_connected{ false };
        bool m_sessionActive{ false };
        struct MonitoredItemInfo
        {
            uint32_t subscriptionId;
            std::shared_ptr<RawSubscription> stream;
        };
        std::unordered_map<uint32_t, MonitoredItemInfo> m_monitoredItems;
        std::unordered_map<int64_t, uint32_t> m_subscriptionMap;

        struct UA_ClientDeleter
        {
            inline void operator()(UA_Client* client) { UA_Client_delete(client); }
        };
        std::unique_ptr<UA_Client, UA_ClientDeleter> m_client;

        std::recursive_mutex m_mutex;
        std::atomic<bool> m_workerRunning{ false };
        std::jthread m_worker;
    };

} // namespace tlink::drivers
