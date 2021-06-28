#pragma once

#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"

#include <app.hpp>

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
        nlohmann::json readingMetadataJson =
            nlohmann::json::parse(metadataStr, nullptr, false);
        if (readingMetadataJson.is_discarded())
        {
            BMCWEB_LOG_ERROR << "Malformed reading metatadata JSON provided by "
                                "telemetry service";
            return false;
        }

        ReadingMetadata readingMetadata;
        try
        {
            readingMetadata = readingMetadataJson.get<ReadingMetadata>();
        }
        catch (const nlohmann::json::exception& e)
        {
            BMCWEB_LOG_ERROR << "Incorrect reading metatadata JSON provided by "
                                "telemetry service: "
                             << e.what();
            return false;
        }

        std::string metricDefinition =
            std::string(metricDefinitionUri) +
            sensors::toReadingType(
                readingMetadata.sensorDbusPath.parent_path().filename());

        metricValues.push_back({
            {"MetricDefinition",
             nlohmann::json{{"@odata.id", metricDefinition}}},
            {"MetricId", id},
            {"MetricProperty", readingMetadata.sensorRedfishUri},
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
    json["@odata.type"] = "#MetricReport.v1_3_0.MetricReport";
    json["@odata.id"] = telemetry::metricReportUri + id;
    json["Id"] = id;
    json["Name"] = id;
    json["MetricReportDefinition"]["@odata.id"] =
        telemetry::metricReportDefinitionUri + id;

    const TimestampReadings* timestampReadings =
        std::get_if<TimestampReadings>(&var);
    if (!timestampReadings)
    {
        BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
        return false;
    }

    const auto& [timestamp, readings] = *timestampReadings;
    nlohmann::json metricValues = nlohmann::json::array_t();
    if (!fillMetricValues(metricValues, readings))
    {
        return false;
    }

    json["Timestamp"] =
        crow::utility::getDateTime(static_cast<time_t>(timestamp));
    json["MetricValues"] = metricValues;

    return true;
}
} // namespace telemetry

inline void requestRoutesMetricReportCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/")
        .privileges({{"Login"}})
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
        .privileges({{"Login"}})
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
