#include <tlink/drivers/ads.hpp>
#include <print>
#include <AdsLib/AdsLib.h>

namespace tlink::drivers {

AdsDriver::AdsDriver(std::string remoteNetId, std::string ipAddress, uint16_t port)
    : m_remoteNetIdStr(std::move(remoteNetId))
    , m_ipAddress(std::move(ipAddress))
    , m_port(port)
{
    // TODO: Parse m_remoteNetIdStr into m_netId
}

AdsDriver::~AdsDriver() {
    // TODO: Ensure cleanup
}

auto AdsDriver::connect() -> Task<Result<void>> {
    // TODO: Implement ADS connection logic
    // 1. AdsAddRoute(m_netId, m_ipAddress.c_str());
    // 2. m_adsPort = AdsPortOpenEx();
    
    co_return tlink::success();
}

auto AdsDriver::disconnect() -> Task<Result<void>> {
    // TODO: Close port and remove route
    co_return tlink::success();
}

auto AdsDriver::readRaw(std::string_view path) -> Task<Result<std::vector<std::byte>>> {
    // TODO: 
    // 1. Get handle for 'path' (AdsSyncReadWriteReqEx2 with ADSIGRP_SYM_HNDBYNAME)
    // 2. Read data size or data (AdsSyncReadReqEx2)
    // 3. Release handle
    
    std::vector<std::byte> dummy;
    co_return dummy;
}

auto AdsDriver::writeRaw(std::string_view path, const std::vector<std::byte>& data) -> Task<Result<void>> {
    // TODO: Implement Write
    co_return tlink::success();
}

auto AdsDriver::subscribe(std::string_view path) -> Task<Result<std::shared_ptr<DataStream>>> {
    // TODO: Implement Subscription
    // 1. Create DataStream
    // 2. Add Device Notification (AdsSyncAddDeviceNotificationReqEx)
    // 3. In callback, push to stream
    
    auto stream = std::make_shared<DataStream>();
    co_return stream;
}

} // namespace tlink::drivers
