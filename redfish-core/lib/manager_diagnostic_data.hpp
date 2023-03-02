#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "privileges.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "routing.hpp"
#include "statistics.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>

#include <string>

namespace redfish
{

inline void
    afterGetManagerStartTime(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const boost::system::error_code& ec,
                             uint64_t bmcwebResetTime)
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
        BMCWEB_LOG_CRITICAL << "Uptime was negative????";
        messages::internalError(aResp->res);
        return;
    }

    // Floor to the closest millisecond
    using Milli = std::chrono::duration<steady_clock::rep, std::milli>;
    Milli milli = std::chrono::floor<Milli>(runTime);

    using SecondsFloat = std::chrono::duration<double>;
    SecondsFloat sec = std::chrono::duration_cast<SecondsFloat>(milli);

    aResp->res.jsonValue["ServiceRootUptimeSeconds"] = sec.count();
}

inline void
    managerGetServiceRootUptime(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, "org.freedesktop.systemd1",
        "/org/freedesktop/systemd1/unit/bmcweb_2eservice",
        "org.freedesktop.systemd1.Unit", "ActiveEnterTimestampMonotonic",
        std::bind_front(afterGetManagerStartTime, aResp));
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
#ifdef BMCWEB_ENABLE_REDFISH_OEM_HTTP_STATS
    asyncResp->res.jsonValue["Oem"]["OpenBMC"]["HttpStatistics"]["@odata.id"] =
        "/redfish/v1/Managers/bmc/ManagerDiagnosticData/OpenBMCHttpStatistics";
#endif
}

inline void handleManagerDiagnosticDataObmcHttpStatsHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/OpenBMCManagerDiagnosticData/OpenBMCManagerDiagnosticData.json>; rel=describedby");
}

inline void handleManagerDiagnosticDataObmcHttpStatsGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    nlohmann::json& j = asyncResp->res.jsonValue;
    j["@odata.id"] =
        "/redfish/v1/Managers/bmc/ManagerDiagnosticData/OpenBMCHttpStatistics";
    j["@odata.type"] =
        "#OpenBMCManagerDiagnosticData.v1_0_0.ConnectionStatistics";
    j["Id"] = "OpenBMCHttpStatistics";
    j["Name"] = "OpenBMC Http Statistics";
    j["Opened"] = bmcweb::stats().opened;
    j["Closed"] = bmcweb::stats().closed;
    j["BytesSent"] = bmcweb::stats().bytesSent;
    j["BytesReceived"] = bmcweb::stats().bytesReceived;
    j["RequestsReceived"] = bmcweb::stats().requestCount;
    j["ResponsesSent"] = bmcweb::stats().responseCount;
    nlohmann::json::array_t responseCounts;
    for (const std::pair<boost::beast::http::status, unsigned>& count :
         bmcweb::stats().responseStatus)
    {
        nlohmann::json::object_t countObj;
        countObj["HttpCode"] = static_cast<unsigned>(count.first);
        countObj["Count"] = static_cast<unsigned>(count.second);
        responseCounts.emplace_back(std::move(countObj));
    }
    j["ResponseCodes"] = std::move(responseCounts);
    j["TLSHandshakeLatencyMs"] = bmcweb::stats().tlsHandshakeTotalMs.count();
    j["RequestReadLatencyMs"] = bmcweb::stats().requestLatencyTotalMs.count();
    j["ProcessingLatencyMs"] = bmcweb::stats().processingLatencyTotalMs.count();
    j["RequestProcessingLatencyMs"] =
        bmcweb::stats().responseWriteLatencyTotalMs.count();
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerDiagnosticDataGet, std::ref(app)));

#ifdef BMCWEB_ENABLE_REDFISH_OEM_HTTP_STATS
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/ManagerDiagnosticData/OpenBMCHttpStatistics")
        .privileges(redfish::privileges::headManagerDiagnosticData)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleManagerDiagnosticDataObmcHttpStatsHead, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/bmc/ManagerDiagnosticData/OpenBMCHttpStatistics")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagerDiagnosticDataObmcHttpStatsGet, std::ref(app)));
#endif
}

} // namespace redfish
