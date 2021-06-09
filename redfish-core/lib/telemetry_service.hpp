#pragma once

#include "utils/telemetry_utils.hpp"

#include <generatedStatic/dataModel/TelemetryService_v1.h>
#include <generatedStatic/serialize/json_telemetryservice.h>

#include <app.hpp>

#include <chrono>
#include <variant>

namespace redfish
{

inline void requestRoutesTelemetryService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/")
        .privileges({{"Login"}})
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            TelemetryServiceV1TelemetryService telemetryService;
            telemetryService.type = "#TelemetryService.v1_2_1.TelemetryService";
            telemetryService.odata = "/redfish/v1/TelemetryService";
            telemetryService.id = "TelemetryService";
            telemetryService.name = "Telemetry Service";
            telemetryService.metricDefinitions.id =
                "/redfish/v1/TelemetryService/MetricReportDefinitions";
            telemetryService.metricReports.id =
                "/redfish/v1/TelemetryService/MetricReports";

            crow::connections::systemBus->async_method_call(
                [asyncResp, telemetryService](
                    const boost::system::error_code ec,
                    const std::vector<std::pair<
                        std::string, std::variant<uint32_t, uint64_t>>>&
                        ret) mutable {
                    if (ec == boost::system::errc::host_unreachable)
                    {
                        telemetryService.status.state = ResourceV1State::Absent;
                        // asyncResp->res.jsonValue["Status"]["State"] =
                        // "Absent";
                        return;
                    }
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    telemetryService.status.state = ResourceV1State::Enabled;
                    // asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                    const size_t* maxReports = nullptr;
                    const uint64_t* minInterval = nullptr;
                    for (const auto& [key, var] : ret)
                    {
                        if (key == "MaxReports")
                        {
                            maxReports = std::get_if<uint64_t>(&var);
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

                    // asyncResp->res.jsonValue["MaxReports"] = *maxReports;
                    telemetryService.maxReports =
                        static_cast<int64_t>(*maxReports);

                    // asyncResp->res.jsonValue["MinCollectionInterval"] =
                    telemetryService.minCollectionInterval =
                        std::chrono::milliseconds(
                            (*minInterval)); // toDurationString
                },
                telemetry::service, "/xyz/openbmc_project/Telemetry/Reports",
                "org.freedesktop.DBus.Properties", "GetAll",
                "xyz.openbmc_project.Telemetry.ReportManager");
            jsonSerializeTelemetryservice(asyncResp, &telemetryService);
        });
}
} // namespace redfish
