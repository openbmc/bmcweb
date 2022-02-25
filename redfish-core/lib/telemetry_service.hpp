#pragma once

#include "utils/telemetry_utils.hpp"

#include <app.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/dbus_utils.hpp>

namespace redfish
{

inline void handleTelemetryServiceGet(
    const crow::Request& /*req*/,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#TelemetryService.v1_2_1.TelemetryService";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/TelemetryService";
    asyncResp->res.jsonValue["Id"] = "TelemetryService";
    asyncResp->res.jsonValue["Name"] = "Telemetry Service";

    asyncResp->res.jsonValue["MetricReportDefinitions"]["@odata.id"] =
        "/redfish/v1/TelemetryService/MetricReportDefinitions";
    asyncResp->res.jsonValue["MetricReports"]["@odata.id"] =
        "/redfish/v1/TelemetryService/MetricReports";
    asyncResp->res.jsonValue["Triggers"]["@odata.id"] =
        "/redfish/v1/TelemetryService/Triggers";

    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, telemetry::service,
        "/xyz/openbmc_project/Telemetry/Reports",
        "xyz.openbmc_project.Telemetry.ReportManager",
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& ret) {
            if (ec == boost::system::errc::host_unreachable)
            {
                asyncResp->res.jsonValue["Status"]["State"] = "Absent";
                return;
            }

            if (ec)
            {
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            const size_t* maxReports = nullptr;
            const uint64_t* minInterval = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), ret, "MaxReports", maxReports,
                "MinInterval", minInterval);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (maxReports)
            {
                asyncResp->res.jsonValue["MaxReports"] = *maxReports;
            }

            if (minInterval)
            {
                asyncResp->res.jsonValue["MinCollectionInterval"] =
                    time_utils::toDurationString(std::chrono::milliseconds(
                        static_cast<time_t>(*minInterval)));
            }

            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        });
}

inline void requestRoutesTelemetryService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/")
        .privileges(redfish::privileges::getTelemetryService)
        .methods(boost::beast::http::verb::get)(handleTelemetryServiceGet);
}

} // namespace redfish
