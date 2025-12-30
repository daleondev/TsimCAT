#include "tlink/drivers/OpcUa.hpp"
#include "tlink/coroutine/Context.hpp"

#include <magic_enum/magic_enum.hpp>
#include <print>

namespace tlink::drivers
{

    UaDriver::UaDriver(std::string endpointUrl)
      : m_endpointUrl(std::move(endpointUrl))
    {
        m_client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(m_client));
    }

    UaDriver::~UaDriver()
    {
        if (m_client) {
            UA_Client_delete(m_client);
            m_client = nullptr;
        }
    }

    auto UaDriver::connect(std::chrono::milliseconds timeout) -> coro::Task<Result<void>>
    {
        if (m_client) {
            // Stub: In a real implementation, we would connect here.
            // UA_StatusCode retval = UA_Client_connect(m_client, m_endpointUrl.c_str());
            // if (retval != UA_STATUSCODE_GOOD) ...
            co_return success();
        }
        co_return std::unexpected(std::make_error_code(std::errc::not_connected));
    }

    auto UaDriver::disconnect(std::chrono::milliseconds timeout) -> coro::Task<Result<void>>
    {
        if (m_client) {
            UA_Client_disconnect(m_client);
        }
        co_return success();
    }

    auto UaDriver::readInto(std::string_view path,
                            std::span<std::byte> dest,
                            std::chrono::milliseconds timeout) -> coro::Task<Result<size_t>>
    {
        // Stub
        co_return 0;
    }

    auto UaDriver::writeFrom(std::string_view path,
                             std::span<const std::byte> src,
                             std::chrono::milliseconds timeout) -> coro::Task<Result<void>>
    {
        // Stub
        co_return success();
    }

    auto UaDriver::subscribeRaw(std::string_view path,
                                SubscriptionType type,
                                std::chrono::milliseconds interval)
      -> coro::Task<Result<std::shared_ptr<RawSubscription>>>
    {
        // Stub
        co_return std::unexpected(std::make_error_code(std::errc::not_supported));
    }

    auto UaDriver::unsubscribeRaw(std::shared_ptr<RawSubscription> subscription) -> coro::Task<Result<void>>
    {
        co_return success();
    }

    auto UaDriver::unsubscribeRawSync(uint64_t id) -> void
    {
    }

} // namespace tlink::drivers
