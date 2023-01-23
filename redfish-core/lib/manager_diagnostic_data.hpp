#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "privileges.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "routing.hpp"

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

    steady_clock::duration run = now - resetTime;

    if (run < steady_clock::duration::zero())
    {
        BMCWEB_LOG_CRITICAL << "Uptime was negative????";
        messages::internalError(aResp->res);
        return;
    }

    // Floor to the closest millisecond
    using Milli = std::chrono::duration<steady_clock::rep, std::milli>;
    Milli milli = std::chrono::floor<Milli>(run);

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
}

inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagerDiagnosticDataGet, std::ref(app)));
}

} // namespace redfish
