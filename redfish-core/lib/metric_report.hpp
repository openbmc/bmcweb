#pragma once

#include "utils/collection.hpp"
#include "utils/telemetry_utils.hpp"

#include <app.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>

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
            {"Timestamp", crow::utility::getDateTimeUint(timestamp)},
        });
    }

    return metricValues;
}

inline bool fillReport(nlohmann::json& json, const std::string& id,
                       const dbus::utility::DbusVariantType& var)
{
    json["@odata.type"] = "#MetricReport.v1_3_0.MetricReport";
    json["@odata.id"] = telemetry::metricReportUri + std::string("/") + id;
    json["Id"] = id;
    json["Name"] = id;
    json["MetricReportDefinition"]["@odata.id"] =
        telemetry::metricReportDefinitionUri + std::string("/") + id;

    const TimestampReadings* timestampReadings =
        std::get_if<TimestampReadings>(&var);
    if (!timestampReadings)
    {
        BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
        return false;
    }

    const auto& [timestamp, readings] = *timestampReadings;
    json["Timestamp"] = crow::utility::getDateTimeUint(timestamp);
    json["MetricValues"] = toMetricValues(readings);
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
                const std::vector<const char*> interfaces{
                    telemetry::reportInterface};
                collection_util::getCollectionMembers(
                    asyncResp, telemetry::metricReportUri, interfaces,
                    "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
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
                                 const dbus::utility::DbusVariantType& ret) {
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
