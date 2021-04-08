#pragma once

#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace telemetry
{

using Readings =
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>;
using TimestampReadings = std::tuple<uint64_t, Readings>;

inline nlohmann::json toMetricValues(const Readings& readings)
{
    nlohmann::json metricValues = nlohmann::json::array_t();

    for (auto& [id, metadata, sensorValue, timestamp] : readings)
    {
        metricValues.push_back({
            {"MetricId", id},
            {"MetricProperty", metadata},
            {"MetricValue", std::to_string(sensorValue)},
            {"Timestamp",
             crow::utility::getDateTime(static_cast<time_t>(timestamp))},
        });
    }

    return metricValues;
}

inline void fillReport(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& id,
                       const std::variant<TimestampReadings>& var)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#MetricReport.v1_3_0.MetricReport";
    asyncResp->res.jsonValue["@odata.id"] = telemetry::metricReportUri + id;
    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["Name"] = id;
    asyncResp->res.jsonValue["MetricReportDefinition"]["@odata.id"] =
        telemetry::metricReportDefinitionUri + id;

    const TimestampReadings* timestampReadings =
        std::get_if<TimestampReadings>(&var);
    if (!timestampReadings)
    {
        BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& [timestamp, readings] = *timestampReadings;
    asyncResp->res.jsonValue["Timestamp"] =
        crow::utility::getDateTime(static_cast<time_t>(timestamp));
    asyncResp->res.jsonValue["MetricValues"] = toMetricValues(readings);
}
} // namespace telemetry

inline void requestRoutesMetricReportCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/")
        .privileges({"Login"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#MetricReportCollection.MetricReportCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TelemetryService/MetricReports";
                asyncResp->res.jsonValue["Name"] = "Metric Report Collection";

                telemetry::getReportCollection(asyncResp,
                                               telemetry::metricReportUri);
            });
}

inline void requestRoutesMetricReport(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/<str>/")
        .privileges({"ConfigureManager"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
                const std::string reportPath = telemetry::getDbusReportPath(id);
                crow::connections::systemBus->async_method_call(
                    [asyncResp, id,
                     reportPath](const boost::system::error_code& ec) {
                        if (ec.value() == EBADR ||
                            ec == boost::system::errc::host_unreachable)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "MetricReport", id);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        crow::connections::systemBus->async_method_call(
                            [asyncResp,
                             id](const boost::system::error_code ec,
                                 const std::variant<
                                     telemetry::TimestampReadings>& ret) {
                                if (ec)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "respHandler DBus error " << ec;
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                telemetry::fillReport(asyncResp, id, ret);
                            },
                            telemetry::service, reportPath,
                            "org.freedesktop.DBus.Properties", "Get",
                            telemetry::reportInterface, "Readings");
                    },
                    telemetry::service, reportPath, telemetry::reportInterface,
                    "Update");
            });
}
} // namespace redfish
