#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "privileges.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "routing.hpp"

#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>

#include <functional>
#include <limits>
#include <memory>
#include <string>

namespace redfish
{

static constexpr auto healthMonitorServiceName =
    "xyz.openbmc_project.HealthMon";
static constexpr auto valueInterface = "xyz.openbmc_project.Metric.Value";
static constexpr auto valueProperty = "Value";
static constexpr double roundFactor = 10000; // 4 decimal places

inline void checkErrors(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const boost::system::error_code& ec,
                        const std::string& message)
{
    if (ec.value() == boost::asio::error::basic_errors::host_unreachable)
    {
        BMCWEB_LOG_WARNING("Failed to find server, Dbus error {}", ec);
    }
    else if (ec.value() == boost::system::linux_error::bad_request_descriptor)
    {
        BMCWEB_LOG_WARNING("Invalid Path, Dbus error {}", ec);
    }
    else
    {
        BMCWEB_LOG_ERROR("{} failed, error {}", message, ec);
        messages::internalError(asyncResp->res);
    }
}

inline void afterGetFreeStorageStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double freeStorage)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(freeStorage))
    {
        asyncResp->res.jsonValue["FreeStorageSpaceKiB"] = nullptr;
        return;
    }
    asyncResp->res.jsonValue["FreeStorageSpaceKiB"] =
        static_cast<int64_t>(freeStorage / 1024.0);
}

inline void managerGetStorageStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr auto freeStorageObjPath =
        "/xyz/openbmc_project/metric/bmc/storage/rw";

    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        freeStorageObjPath, valueInterface, valueProperty,
        std::bind_front(afterGetFreeStorageStatistics, asyncResp));
}

inline void afterGetProcessorKernelStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double kernelCPU)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(kernelCPU))
    {
        asyncResp->res.jsonValue["ProcessorStatistics"]["KernelPercent"] =
            nullptr;
        return;
    }
    asyncResp->res.jsonValue["ProcessorStatistics"]["KernelPercent"] =
        std::round(kernelCPU * roundFactor) / roundFactor;
}

inline void afterGetProcessorUserStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double userCPU)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(userCPU))
    {
        asyncResp->res.jsonValue["ProcessorStatistics"]["UserPercent"] =
            nullptr;
        return;
    }
    asyncResp->res.jsonValue["ProcessorStatistics"]["UserPercent"] =
        std::round(userCPU * roundFactor) / roundFactor;
}

inline void managerGetProcessorStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr auto kernelCPUObjPath =
        "/xyz/openbmc_project/metric/bmc/cpu/kernel";
    constexpr auto userCPUObjPath = "/xyz/openbmc_project/metric/bmc/cpu/user";

    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        kernelCPUObjPath, valueInterface, valueProperty,
        std::bind_front(afterGetProcessorKernelStatistics, asyncResp));

    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName, userCPUObjPath,
        valueInterface, valueProperty,
        std::bind_front(afterGetProcessorUserStatistics, asyncResp));
}

inline void afterGetAvailableMemoryStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double availableMemory)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(availableMemory))
    {
        asyncResp->res.jsonValue["MemoryStatistics"]["AvailableBytes"] =
            nullptr;
        return;
    }
    asyncResp->res.jsonValue["MemoryStatistics"]["AvailableBytes"] =
        static_cast<int64_t>(availableMemory);
}

inline void afterGetBufferedAndCachedMemoryStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double bufferedAndCachedMemory)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(bufferedAndCachedMemory))
    {
        asyncResp->res.jsonValue["MemoryStatistics"]["BuffersAndCacheBytes"] =
            nullptr;
        return;
    }
    asyncResp->res.jsonValue["MemoryStatistics"]["BuffersAndCacheBytes"] =
        static_cast<int64_t>(bufferedAndCachedMemory);
}

inline void afterGetFreeMemoryStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double freeMemory)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(freeMemory))
    {
        asyncResp->res.jsonValue["MemoryStatistics"]["FreeBytes"] = nullptr;
        return;
    }
    asyncResp->res.jsonValue["MemoryStatistics"]["FreeBytes"] =
        static_cast<int64_t>(freeMemory);
}

inline void afterGetSharedMemoryStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double sharedMemory)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(sharedMemory))
    {
        asyncResp->res.jsonValue["MemoryStatistics"]["SharedBytes"] = nullptr;
        return;
    }
    asyncResp->res.jsonValue["MemoryStatistics"]["SharedBytes"] =
        static_cast<int64_t>(sharedMemory);
}

inline void afterGetTotalMemoryStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, double totalMemory)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    if (!std::isfinite(totalMemory))
    {
        asyncResp->res.jsonValue["MemoryStatistics"]["TotalBytes"] = nullptr;
        return;
    }
    asyncResp->res.jsonValue["MemoryStatistics"]["TotalBytes"] =
        static_cast<int64_t>(totalMemory);
}

inline void managerGetMemoryStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr auto availableMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/available";
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        availableMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(afterGetAvailableMemoryStatistics, asyncResp));

    constexpr auto bufferedAndCachedMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/buffered_and_cached";
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        bufferedAndCachedMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(afterGetBufferedAndCachedMemoryStatistics, asyncResp));

    constexpr auto freeMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/free";
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        freeMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(afterGetFreeMemoryStatistics, asyncResp));

    constexpr auto sharedMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/shared";
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        sharedMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(afterGetSharedMemoryStatistics, asyncResp));

    constexpr auto totalMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/total";
    sdbusplus::asio::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        totalMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(afterGetTotalMemoryStatistics, asyncResp));
}

inline void afterGetManagerStartTime(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, uint64_t bmcwebResetTime)
{
    if (ec)
    {
        checkErrors(asyncResp, ec, __func__);
        return;
    }
    using std::chrono::steady_clock;

    std::chrono::duration<steady_clock::rep, std::micro> usReset{
        bmcwebResetTime};
    steady_clock::time_point resetTime{usReset};

    steady_clock::time_point now = steady_clock::now();

    steady_clock::duration runTime = now - resetTime;

    if (runTime < steady_clock::duration::zero())
    {
        BMCWEB_LOG_CRITICAL("Uptime was negative????");
        messages::internalError(asyncResp->res);
        return;
    }

    // Floor to the closest millisecond
    using Milli = std::chrono::duration<steady_clock::rep, std::milli>;
    Milli milli = std::chrono::floor<Milli>(runTime);

    using SecondsFloat = std::chrono::duration<double>;
    SecondsFloat sec = std::chrono::duration_cast<SecondsFloat>(milli);

    asyncResp->res.jsonValue["ServiceRootUptimeSeconds"] = sec.count();
}

inline void managerGetServiceRootUptime(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, "org.freedesktop.systemd1",
        "/org/freedesktop/systemd1/unit/bmcweb_2eservice",
        "org.freedesktop.systemd1.Unit", "ActiveEnterTimestampMonotonic",
        std::bind_front(afterGetManagerStartTime, asyncResp));
}
/**
 * handleManagerDiagnosticData supports ManagerDiagnosticData.
 * It retrieves BMC health information from various DBus resources and returns
 * the information through the response.
 */
inline void handleManagerDiagnosticDataGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#ManagerDiagnosticData.v1_2_0.ManagerDiagnosticData";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/ManagerDiagnosticData";
    asyncResp->res.jsonValue["Id"] = "ManagerDiagnosticData";
    asyncResp->res.jsonValue["Name"] = "Manager Diagnostic Data";

    managerGetServiceRootUptime(asyncResp);
    managerGetProcessorStatistics(asyncResp);
    managerGetMemoryStatistics(asyncResp);
    managerGetStorageStatistics(asyncResp);
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerDiagnosticDataGet, std::ref(app)));
}

} // namespace redfish
