#pragma once

#include "utils/telemetry_utils.hpp"

#include <app.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>

#include <variant>

namespace redfish
{

inline void handleTelemetryServiceGet(
    const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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

    crow::connections::systemBus->async_method_call(
        [asyncResp](
            const boost::system::error_code ec,
            const std::vector<
                std::pair<std::string, dbus::utility::DbusVariantType>>& ret) {
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

            asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

            const size_t* maxReports = nullptr;
            const uint64_t* minInterval = nullptr;
            for (const auto& [key, var] : ret)
            {
                if (key == "MaxReports")
                {
                    maxReports = std::get_if<size_t>(&var);
                }
                else if (key == "MinInterval")
                {
                    minInterval = std::get_if<uint64_t>(&var);
                }
            }
            if (!maxReports || !minInterval)
            {
                BMCWEB_LOG_ERROR
                    << "Property type mismatch or property is missing";
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.jsonValue["MaxReports"] = *maxReports;
            asyncResp->res.jsonValue["MinCollectionInterval"] =
                time_utils::toDurationString(std::chrono::milliseconds(
                    static_cast<time_t>(*minInterval)));
        },
        telemetry::service, "/xyz/openbmc_project/Telemetry/Reports",
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Telemetry.ReportManager");
}

inline void requestRoutesTelemetryService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/")
        .privileges(redfish::privileges::getTelemetryService)
        .methods(boost::beast::http::verb::get)(handleTelemetryServiceGet);
}

} // namespace redfish
