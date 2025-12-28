#include "tlink/drivers/ads.hpp"

#include <print>
#include <atomic>
#include <ranges>
#include <cassert>
#include <iostream>

namespace
{
    static std::atomic_bool s_localNetIdSet{false};
    static auto strToNetId(std::string_view str) -> AmsNetId
    {
        auto parts{str | std::views::split('.')};
        AmsNetId netId{};

        auto i{0uz};
        for (auto &&part : parts)
        {
            assert(i < 6 && "Invalid NetId string");
            std::from_chars(part.data(), part.data() + part.size(), netId.b[i]);
            ++i;
        }
        return netId;
    }

    using AdsErrorType = std::remove_cvref_t<decltype(AdsException::errorCode)>;
    enum class AdsError : AdsErrorType
    {
        None = 0x00, // ADSERR_NOERR

        // --- Device Errors (Base: 0x0700) ---
        DeviceError = 0x700,                /**< Error class < device error > */
        DeviceServiceNotSupported = 0x701,  /**< Service is not supported by server */
        DeviceInvalidGroup = 0x702,         /**< invalid indexGroup */
        DeviceInvalidOffset = 0x703,        /**< invalid indexOffset */
        DeviceInvalidAccess = 0x704,        /**< reading/writing not permitted */
        DeviceInvalidSize = 0x705,          /**< parameter size not correct */
        DeviceInvalidData = 0x706,          /**< invalid parameter value(s) */
        DeviceNotReady = 0x707,             /**< device is not in a ready state */
        DeviceBusy = 0x708,                 /**< device is busy */
        DeviceInvalidContext = 0x709,       /**< invalid context (must be InWindows) */
        DeviceNoMemory = 0x70A,             /**< out of memory */
        DeviceInvalidParam = 0x70B,         /**< invalid parameter value(s) */
        DeviceNotFound = 0x70C,             /**< not found (files, ...) */
        DeviceSyntaxError = 0x70D,          /**< syntax error in command or file */
        DeviceIncompatible = 0x70E,         /**< objects do not match */
        DeviceExists = 0x70F,               /**< object already exists */
        DeviceSymbolNotFound = 0x710,       /**< symbol not found */
        DeviceSymbolVersionInvalid = 0x711, /**< symbol version invalid, possibly caused by 'onlinechange' */
        DeviceInvalidState = 0x712,         /**< server is in invalid state */
        DeviceTransModeNotSupp = 0x713,     /**< AdsTransMode not supported */
        DeviceNotifyHandleInvalid = 0x714,  /**< Notification handle is invalid */
        DeviceClientUnknown = 0x715,        /**< Notification client not registered */
        DeviceNoMoreHandles = 0x716,        /**< no more notification handles */
        DeviceInvalidWatchSize = 0x717,     /**< size for watch too big */
        DeviceNotInitialized = 0x718,       /**< device not initialized */
        DeviceTimeout = 0x719,              /**< device has a timeout */
        DeviceNoInterface = 0x71A,          /**< query interface failed */
        DeviceInvalidInterface = 0x71B,     /**< wrong interface required */
        DeviceInvalidClsId = 0x71C,         /**< class ID is invalid */
        DeviceInvalidObjId = 0x71D,         /**< object ID is invalid */
        DevicePending = 0x71E,              /**< request is pending */
        DeviceAborted = 0x71F,              /**< request is aborted */
        DeviceWarning = 0x720,              /**< signal warning */
        DeviceInvalidArrayIndex = 0x721,    /**< invalid array index */
        DeviceSymbolNotActive = 0x722,      /**< symbol not active */
        DeviceAccessDenied = 0x723,         /**< access denied */
        DeviceLicenseNotFound = 0x724,      /**< no license found */
        DeviceLicenseExpired = 0x725,       /**< license expired */
        DeviceLicenseExceeded = 0x726,      /**< license exceeded */
        DeviceLicenseInvalid = 0x727,       /**< license invalid */
        DeviceLicenseSystemId = 0x728,      /**< license invalid system id */
        DeviceLicenseNoTimeLimit = 0x729,   /**< license not time limited */
        DeviceLicenseFutureIssue = 0x72A,   /**< license issue time in the future */
        DeviceLicenseTimeTooLong = 0x72B,   /**< license time period too long */
        DeviceException = 0x72C,            /**< exception in device specific code */
        DeviceLicenseDuplicated = 0x72D,    /**< license file read twice */
        DeviceSignatureInvalid = 0x72E,     /**< invalid signature */
        DeviceCertificateInvalid = 0x72F,   /**< public key certificate */

        // --- Client Errors (Base: 0x0740) ---
        ClientError = 0x740,             /**< Error class < client error > */
        ClientInvalidParam = 0x741,      /**< invalid parameter at service call */
        ClientListEmpty = 0x742,         /**< polling list is empty */
        ClientVarUsed = 0x743,           /**< var connection already in use */
        ClientDuplicateInvokeId = 0x744, /**< invoke id in use */
        ClientSyncTimeout = 0x745,       /**< timeout elapsed */
        ClientW32Error = 0x746,          /**< error in win32 subsystem */
        ClientTimeoutInvalid = 0x747,    /**< Invalid client timeout value */
        ClientPortNotOpen = 0x748,       /**< ads dll */
        ClientNoAmsAddr = 0x749,         /**< ads dll */
        ClientSyncInternal = 0x750,      /**< internal error in ads sync */
        ClientAddHash = 0x751,           /**< hash table overflow */
        ClientRemoveHash = 0x752,        /**< key not found in hash table */
        ClientNoMoreSym = 0x753,         /**< no more symbols in cache */
        ClientSyncResInvalid = 0x754,    /**< invalid response received */
        ClientSyncPortLocked = 0x755,    /**< sync port is locked */

        Unknown = std::numeric_limits<AdsErrorType>::max()
    };

    class AdsErrorCategory : public std::error_category
    {
    public:
        const char *name() const noexcept override
        {
            return "AdsError";
        }

        std::string message(int ev) const override
        {
            (void)static_cast<AdsError>(ev);
            return "todo with magic_enum";
        }
    };

    const std::error_category &ads_category()
    {
        static AdsErrorCategory instance;
        return instance;
    }

    std::error_code make_error_code(AdsError e)
    {
        return std::error_code(
            static_cast<int>(e),
            ads_category());
    }

    static AdsError handleException(const std::exception &ex)
    {
        auto *adsEx{dynamic_cast<const AdsException *>(&ex)};
        if (adsEx)
        {
            std::println(std::cerr, "Ads Exception: {}", adsEx->what());
            return static_cast<AdsError>(adsEx->errorCode);
        }
        std::println(std::cerr, "Unknown Exception: {}", ex.what());
        return AdsError::Unknown;
    }
}

namespace std
{
    template <>
    struct is_error_code_enum<AdsError> : true_type
    {
    };
}

namespace tlink::drivers
{

    AdsDriver::AdsDriver(std::string_view remoteNetId, std::string ipAddress, uint16_t port, std::string_view localNetId)
        : m_remoteNetId{strToNetId(remoteNetId)}, m_ipAddress(std::move(ipAddress)), m_port{port}, m_route{nullptr}, m_localNetId{strToNetId(localNetId)}
    {
        if (!localNetId.empty())
        {
            bhf::ads::SetLocalAddress(m_localNetId);
        }
    }

    AdsDriver::~AdsDriver()
    {
        (void)disconnect();
    }

    auto AdsDriver::connect() -> Task<Result<void>>
    {
        auto err{AdsError::None};
        try
        {
            m_route = std::make_unique<AdsDevice>(m_ipAddress, m_remoteNetId, m_port);
            (void)m_route->GetDeviceInfo();
        }
        catch (const std::exception &ex)
        {
            err = handleException(ex);
        }

        co_return err == AdsError::None ? success() : std::unexpected(make_error_code(err));
    }

    auto AdsDriver::disconnect() -> Task<Result<void>>
    {
        if (!m_route)
        {
            co_return success();
        }

        auto err{AdsError::None};
        try
        {
            m_handles.clear();
            m_route.reset();
        }
        catch (const std::exception &ex)
        {
            err = handleException(ex);
        }

        co_return err == AdsError::None ? success() : std::unexpected(make_error_code(err));
    }

    auto AdsDriver::readInto(std::string_view path, std::span<std::byte> dest) -> Task<Result<size_t>>
    {
        uint32_t bytesRead = 0;
        auto err{AdsError::None};
        try
        {
            auto handle{m_route->GetHandle(std::string(path))};
            err = static_cast<AdsError>(m_route->ReadReqEx2(ADSIGRP_SYM_VALBYHND, *handle, dest.size(),
                                                            dest.data(), &bytesRead));
        }
        catch (const std::exception &ex)
        {
            err = handleException(ex);
        }

        if (err != AdsError::None)
        {
            co_return std::unexpected(make_error_code(err));
        }

        co_return bytesRead;
    }

    auto AdsDriver::writeFrom(std::string_view path, std::span<const std::byte> src) -> Task<Result<void>>
    {
        auto err{AdsError::None};
        try
        {
            auto handle{m_route->GetHandle(std::string(path))};
            err = static_cast<AdsError>(m_route->WriteReqEx(ADSIGRP_SYM_VALBYHND, *handle, src.size(), src.data()));
        }
        catch (const std::exception &ex)
        {
            err = handleException(ex);
        }

        co_return err == AdsError::None ? success() : std::unexpected(make_error_code(err));
    }

    // auto AdsDriver::readRaw(std::string_view path) -> Task<Result<std::vector<std::byte>>>
    // {
    //     uint32_t bytesRead = 0;
    //     std::vector<std::byte> data(1024);

    //     auto err{AdsError::None};
    //     try
    //     {
    //         auto handle{m_route->GetHandle(std::string(path))};
    //         err = static_cast<AdsError>(m_route->ReadReqEx2(ADSIGRP_SYM_VALBYHND, *handle, data.size(),
    //                                                         data.data(), &bytesRead));
    //     }
    //     catch (const std::exception &ex)
    //     {
    //         err = handleException(ex);
    //     }

    //     if (err != AdsError::None)
    //     {
    //         co_return std::unexpected(make_error_code(err));
    //     }

    //     data.resize(bytesRead);
    //     co_return data;
    // }

    // auto AdsDriver::writeRaw(std::string_view path, const std::vector<std::byte> &data) -> Task<Result<void>>
    // {
    //     auto err{AdsError::None};
    //     try
    //     {
    //         auto handle{m_route->GetHandle(std::string(path))};
    //         err = static_cast<AdsError>(m_route->WriteReqEx(ADSIGRP_SYM_VALBYHND, *handle, data.size(), data.data()));
    //     }
    //     catch (const std::exception &ex)
    //     {
    //         err = handleException(ex);
    //     }

    //     co_return err == AdsError::None ? success() : std::unexpected(make_error_code(err));
    // }

    auto AdsDriver::subscribe(std::string_view path) -> Task<Result<std::shared_ptr<DataStream>>>
    {
        const AdsNotificationAttrib attrib = {
            1024, ADSTRANS_SERVERCYCLE, 0, {4000000}};

        auto stream = std::make_shared<DataStream>();
        auto handle{m_route->GetHandle(ADSIGRP_SYM_VALBYHND, *m_route->GetHandle(std::string(path)), attrib, [](const AmsAddr *pAddr, const AdsNotificationHeader *pNotification, uint32_t hUser)
                                       { auto* stream{reinterpret_cast<DataStream*>(static_cast<uintptr_t>(hUser))};
                                       stream->push({}); }, reinterpret_cast<uintptr_t>(stream.get()))};
        m_handles.emplace(*handle, std::move(handle));
        co_return stream;
    }

} // namespace tlink::drivers
