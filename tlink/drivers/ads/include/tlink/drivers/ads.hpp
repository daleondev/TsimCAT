#pragma once

#include "tlink/driver.hpp"

#include "AdsLib/AdsLib.h"
#include "AdsLib/AdsNotificationOOI.h"
#include "AdsLib/AdsVariable.h"

#include <string>

namespace tlink::drivers
{

    class AdsDriver : public tlink::IDriver
    {
    public:
        /**
         * @brief Construct a new Ads Driver.
         * @param remoteNetId The NetID of the target PLC (e.g. "127.0.0.1.1.1").
         * @param ipAddress The IP address of the target (e.g. "127.0.0.1").
         * @param port The ADS port (default 851 for PLC1).
         */
        AdsDriver(std::string_view remoteNetId, std::string ipAddress, uint16_t port = AMSPORT_R0_PLC_TC3, std::string_view localNetId = "0.0.0.0.0.0");
        ~AdsDriver() override;

        // IDriver Interface Implementation
        auto connect() -> Task<Result<void>> override;
        auto disconnect() -> Task<Result<void>> override;

        template <typename T>
        auto read(std::string_view path) -> Task<Result<T>>
        {
            AdsVariable<T> readVar{*m_route, std::string(path)};
            co_return readVar;
        }

        template <typename T>
        auto write(std::string_view path, const T &value) -> Task<Result<void>>
        {
            AdsVariable<T> writeVar{*m_route, std::string(path)};
            writeVar = value;
            co_return tlink::success();
        }

        auto readRaw(std::string_view path) -> Task<Result<std::vector<std::byte>>> override;
        auto writeRaw(std::string_view path, const std::vector<std::byte> &data) -> Task<Result<void>> override;

        auto subscribe(std::string_view path) -> Task<Result<std::shared_ptr<DataStream>>> override;

    private:
        AmsNetId m_remoteNetId;
        std::string m_ipAddress;
        uint16_t m_port;
        AmsNetId m_localNetId;

        std::unique_ptr<AdsDevice>
            m_route;

        std::unordered_map<uint32_t, AdsHandle> m_handles;
    };

} // namespace tlink::drivers
