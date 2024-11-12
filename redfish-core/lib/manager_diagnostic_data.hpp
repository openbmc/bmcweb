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
#include <boost/url/format.hpp>
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

inline bool checkErrors(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const std::source_location src = std::source_location::current())
{
    if (ec.value() == boost::asio::error::basic_errors::host_unreachable)
    {
        BMCWEB_LOG_WARNING("Failed to find server, Dbus error {}", ec);
        return true;
    }
    if (ec.value() == boost::system::linux_error::bad_request_descriptor)
    {
        BMCWEB_LOG_WARNING("Invalid Path, Dbus error {}", ec);
        return true;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("{} failed, error {}", src.function_name(), ec);
        messages::internalError(asyncResp->res);
        return true;
    }
    return false;
}

inline void
    setBytesProperty(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const nlohmann::json::json_pointer& jPtr,
                     const boost::system::error_code& ec, double bytes)
{
    if (checkErrors(asyncResp, ec))
    {
        return;
    }
    if (!std::isfinite(bytes))
    {
        BMCWEB_LOG_WARNING("Property read for {} was not finite",
                           jPtr.to_string());
        asyncResp->res.jsonValue[jPtr] = nullptr;
        return;
    }
    // If the param is in Kib, make it Kib.  Redfish uses this as a naming
    // DBus represents as bytes
    if (std::string_view(jPtr.back()).ends_with("KiB"))
    {
        bytes /= 1024.0;
    }

    asyncResp->res.jsonValue[jPtr] = static_cast<int64_t>(bytes);
}

inline void managerGetStorageStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr auto freeStorageObjPath =
        "/xyz/openbmc_project/metric/bmc/storage/rw";

    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        freeStorageObjPath, valueInterface, valueProperty,
        std::bind_front(setBytesProperty, asyncResp,
                        nlohmann::json::json_pointer("/FreeStorageSpaceKiB")));
}

inline void
    setPercentProperty(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const nlohmann::json::json_pointer& jPtr,
                       const boost::system::error_code& ec, double userCPU)
{
    if (checkErrors(asyncResp, ec))
    {
        return;
    }
    if (!std::isfinite(userCPU))
    {
        asyncResp->res.jsonValue[jPtr] = nullptr;
        return;
    }

    static constexpr double roundFactor = 10000; // 4 decimal places
    asyncResp->res.jsonValue[jPtr] =
        std::round(userCPU * roundFactor) / roundFactor;
}

inline void managerGetProcessorStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    constexpr auto kernelCPUObjPath =
        "/xyz/openbmc_project/metric/bmc/cpu/kernel";
    constexpr auto userCPUObjPath = "/xyz/openbmc_project/metric/bmc/cpu/user";

    using json_pointer = nlohmann::json::json_pointer;
    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        kernelCPUObjPath, valueInterface, valueProperty,
        std::bind_front(setPercentProperty, asyncResp,
                        json_pointer("/ProcessorStatistics/KernelPercent")));

    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName, userCPUObjPath,
        valueInterface, valueProperty,
        std::bind_front(setPercentProperty, asyncResp,
                        json_pointer("/ProcessorStatistics/UserPercent")));
}

inline void managerGetMemoryStatistics(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    using json_pointer = nlohmann::json::json_pointer;
    constexpr auto availableMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/available";
    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        availableMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(setBytesProperty, asyncResp,
                        json_pointer("/MemoryStatistics/AvailableBytes")));

    constexpr auto bufferedAndCachedMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/buffered_and_cached";
    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        bufferedAndCachedMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(
            setBytesProperty, asyncResp,
            json_pointer("/MemoryStatistics/BuffersAndCacheBytes")));

    constexpr auto freeMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/free";
    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        freeMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(setBytesProperty, asyncResp,
                        json_pointer("/MemoryStatistics/FreeBytes")));

    constexpr auto sharedMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/shared";
    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        sharedMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(setBytesProperty, asyncResp,
                        json_pointer("/MemoryStatistics/SharedBytes")));

    constexpr auto totalMemoryObjPath =
        "/xyz/openbmc_project/metric/bmc/memory/total";
    dbus::utility::getProperty<double>(
        *crow::connections::systemBus, healthMonitorServiceName,
        totalMemoryObjPath, valueInterface, valueProperty,
        std::bind_front(setBytesProperty, asyncResp,
                        json_pointer("/MemoryStatistics/TotalBytes")));
}

inline void afterGetManagerStartTime(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, uint64_t bmcwebResetTime)
{
    if (ec)
    {
        // Not all servers will be running in systemd, so ignore the error.
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
    dbus::utility::getProperty<uint64_t>(
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
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#ManagerDiagnosticData.v1_2_0.ManagerDiagnosticData";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/ManagerDiagnosticData",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
    asyncResp->res.jsonValue["Id"] = "ManagerDiagnosticData";
    asyncResp->res.jsonValue["Name"] = "Manager Diagnostic Data";

    managerGetServiceRootUptime(asyncResp);
    managerGetProcessorStatistics(asyncResp);
    managerGetMemoryStatistics(asyncResp);
    managerGetStorageStatistics(asyncResp);
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerDiagnosticDataGet, std::ref(app)));
}

} // namespace redfish
