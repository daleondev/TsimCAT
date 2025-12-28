#pragma once
#include <tlink/driver.hpp>
#include <AdsLib/AdsLib.h> // Safe now, thanks to our CMake fix
#include <string>

namespace tlink::drivers {

class AdsDriver : public tlink::IDriver {
public:
    /**
     * @brief Construct a new Ads Driver.
     * @param remote_net_id The NetID of the target PLC (e.g. "127.0.0.1.1.1").
     * @param ip_address The IP address of the target (e.g. "127.0.0.1").
     * @param port The ADS port (default 851 for PLC1).
     */
    AdsDriver(std::string remote_net_id, std::string ip_address, uint16_t port = 851);
    ~AdsDriver() override;

    // IDriver Interface Implementation
    auto connect() -> Task<Result<void>> override;
    auto disconnect() -> Task<Result<void>> override;
    
    auto readRaw(std::string_view path) -> Task<Result<std::vector<std::byte>>> override;
    auto writeRaw(std::string_view path, const std::vector<std::byte>& data) -> Task<Result<void>> override;
    
    auto subscribe(std::string_view path) -> Task<Result<std::shared_ptr<DataStream>>> override;

private:
    std::string m_remoteNetIdStr;
    std::string m_ipAddress;
    uint16_t m_port;
    
    AmsNetId m_netId;
    long m_adsPort = 0; // The local port handle returned by AdsPortOpenEx
};

} // namespace tlink::drivers
