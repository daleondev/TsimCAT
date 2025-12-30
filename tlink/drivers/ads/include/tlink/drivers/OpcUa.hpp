#pragma once

#include "tlink/Driver.hpp"

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>

#include <string>

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
                       SubscriptionType type = SubscriptionType::OnChange,
                       std::chrono::milliseconds interval = NO_TIMEOUT) -> coro::Task<Result<std::shared_ptr<RawSubscription>>> override;
        auto unsubscribeRaw(std::shared_ptr<RawSubscription> subscription) -> coro::Task<Result<void>> override;
        auto unsubscribeRawSync(uint64_t id) -> void override;
        // clang-format on

      private:
        std::string m_endpointUrl;
        UA_Client* m_client = nullptr;
    };

} // namespace tlink::drivers
