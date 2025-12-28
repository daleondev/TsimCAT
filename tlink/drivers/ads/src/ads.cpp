#include <tlink/drivers/ads.hpp>
#include <print>
#include <AdsLib/AdsLib.h>

namespace tlink::drivers {

AdsDriver::AdsDriver(std::string remote_net_id, std::string ip_address, uint16_t port)
    : m_remote_net_id_str(std::move(remote_net_id))
    , m_ip_address(std::move(ip_address))
    , m_port(port)
{
    // TODO: Parse m_remote_net_id_str into m_net_id
}

AdsDriver::~AdsDriver() {
    // TODO: Ensure cleanup
}

auto AdsDriver::connect() -> Task<Result<void>> {
    // TODO: Implement ADS connection logic
    // 1. AdsAddRoute(m_net_id, m_ip_address.c_str());
    // 2. m_ads_port = AdsPortOpenEx();
    
    co_return tlink::success();
}

auto AdsDriver::disconnect() -> Task<Result<void>> {
    // TODO: Close port and remove route
    co_return tlink::success();
}

auto AdsDriver::read_raw(std::string_view path) -> Task<Result<std::vector<std::byte>>> {
    // TODO: 
    // 1. Get handle for 'path' (AdsSyncReadWriteReqEx2 with ADSIGRP_SYM_HNDBYNAME)
    // 2. Read data size or data (AdsSyncReadReqEx2)
    // 3. Release handle
    
    std::vector<std::byte> dummy;
    co_return dummy;
}

auto AdsDriver::write_raw(std::string_view path, const std::vector<std::byte>& data) -> Task<Result<void>> {
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
