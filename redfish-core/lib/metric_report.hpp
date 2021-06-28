#pragma once

#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"

#include <app.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{

namespace telemetry
{

using Readings =
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>;
using TimestampReadings = std::tuple<uint64_t, Readings>;

inline bool fillMetricValues(nlohmann::json& metricValues,
                             const Readings& readings)
{
    for (auto& [id, metadataStr, sensorValue, timestamp] : readings)
    {
        std::optional<nlohmann::json> readingMetadataJson =
            getMetadataJson(metadataStr);
        if (!readingMetadataJson)
        {
            return false;
        }

        std::optional<std::string> sensorDbusPath =
            readStringFromMetadata(*readingMetadataJson, "SensorDbusPath");
        if (!sensorDbusPath)
        {
            return false;
        }

        std::optional<std::string> sensorRedfishUri =
            readStringFromMetadata(*readingMetadataJson, "SensorRedfishUri");
        if (!sensorRedfishUri)
        {
            return false;
        }

        std::string metricDefinition =
            std::string(metricDefinitionUri) +
            sensors::toReadingType(
                sdbusplus::message::object_path(*sensorDbusPath)
                    .parent_path()
                    .filename());

        metricValues.push_back({
            {"MetricDefinition",
             nlohmann::json{{"@odata.id", metricDefinition}}},
            {"MetricId", id},
            {"MetricProperty", *sensorRedfishUri},
            {"MetricValue", std::to_string(sensorValue)},
            {"Timestamp",
             crow::utility::getDateTime(static_cast<time_t>(timestamp))},
        });
    }

    return true;
}

inline bool fillReport(nlohmann::json& json, const std::string& id,
                       const std::variant<TimestampReadings>& var)
{
    const TimestampReadings* timestampReadings =
        std::get_if<TimestampReadings>(&var);
    if (!timestampReadings)
    {
        BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
        return false;
    }

    const auto& [timestamp, readings] = *timestampReadings;
    nlohmann::json metricValues = nlohmann::json::array();
    if (!fillMetricValues(metricValues, readings))
    {
        return false;
    }

    json["@odata.type"] = "#MetricReport.v1_3_0.MetricReport";
    json["@odata.id"] = telemetry::metricReportUri + id;
    json["Id"] = id;
    json["Name"] = id;
    json["MetricReportDefinition"]["@odata.id"] =
        telemetry::metricReportDefinitionUri + id;
    json["Timestamp"] =
        crow::utility::getDateTime(static_cast<time_t>(timestamp));
    json["MetricValues"] = metricValues;

    return true;
}
} // namespace telemetry

inline void requestRoutesMetricReportCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/")
        .privileges(redfish::privileges::getMetricReportCollection)
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
        .privileges(redfish::privileges::getMetricReport)
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

                                if (!telemetry::fillReport(
                                        asyncResp->res.jsonValue, id, ret))
                                {
                                    messages::internalError(asyncResp->res);
                                }
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
