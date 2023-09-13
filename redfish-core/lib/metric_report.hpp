#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>

#include <array>
#include <string_view>

namespace redfish
{

namespace telemetry
{

using Readings = std::vector<std::tuple<std::string, double, uint64_t>>;
using TimestampReadings = std::tuple<uint64_t, Readings>;

inline nlohmann::json toMetricValues(const Readings& readings)
{
    nlohmann::json metricValues = nlohmann::json::array_t();

    for (const auto& [metadata, sensorValue, timestamp] : readings)
    {
        nlohmann::json::object_t metricReport;
        metricReport["MetricProperty"] = metadata;
        metricReport["MetricValue"] = std::to_string(sensorValue);
        metricReport["Timestamp"] =
            redfish::time_utils::getDateTimeUintMs(timestamp);
        metricValues.emplace_back(std::move(metricReport));
    }

    return metricValues;
}

inline bool fillReport(nlohmann::json& json, const std::string& id,
                       const TimestampReadings& timestampReadings)
{
    json["@odata.type"] = "#MetricReport.v1_3_0.MetricReport";
    json["@odata.id"] = boost::urls::format(
        "/redfish/v1/TelemetryService/MetricReports/{}", id);
    json["Id"] = id;
    json["Name"] = id;
    json["MetricReportDefinition"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/TelemetryService/MetricReportDefinitions/{}", id);

    const auto& [timestamp, readings] = timestampReadings;
    json["Timestamp"] = redfish::time_utils::getDateTimeUintMs(timestamp);
    json["MetricValues"] = toMetricValues(readings);
    return true;
}
} // namespace telemetry

inline void requestRoutesMetricReportCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/")
        .privileges(redfish::privileges::getMetricReportCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#MetricReportCollection.MetricReportCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReports";
        asyncResp->res.jsonValue["Name"] = "Metric Report Collection";
        constexpr std::array<std::string_view, 1> interfaces{
            telemetry::reportInterface};
        collection_util::getCollectionMembers(
            asyncResp,
            boost::urls::url("/redfish/v1/TelemetryService/MetricReports"),
            interfaces,
            "/xyz/openbmc_project/Telemetry/Reports/TelemetryService");
        });
}

inline void
    getPlatforMetrics(const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& metricId,
                      const uint64_t& requestTimestamp = 0)
{
    boost::asio::post(crow::connections::getNextIoContext(),
                      [req, asyncResp]() {
        BMCWEB_LOG_INFO("Populating MRD Response");
        nlohmann::json& resArray = asyncResp->res.jsonValue["MetricValues"];

        for (int i = 0; i < 5000; i++)
        {
            nlohmann::json thisMetric = nlohmann::json::object();
            thisMetric["MetricValue"] = 0;
            thisMetric["MetricProperty"] =
                "/redfish/v1/Fabrics/HGX_NVLinkFabric_0/Switches/NVSwitch_0/Ports/NVLink_0#/Oem/Nvidia/RXWidth";
            thisMetric["Timestamp"] = "2020-03-27T16:50:58.516+00:00";
            resArray.push_back(thisMetric);
        }

        boost::asio::post(*req.ioService, [asyncResp]() {
            messages::success(asyncResp->res); // Not needed but preventing any optimization 
            BMCWEB_LOG_INFO("Sending MRD Response");
        });
    });

    return;
}

inline void requestRoutesMetricReport(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricReports/<str>/")
        .privileges(redfish::privileges::getMetricReport)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        // Test code for Multithread model 
        const uint64_t requestTimestamp = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
        getPlatforMetrics(req, asyncResp, id, requestTimestamp);
    
        const std::string reportPath = telemetry::getDbusReportPath(id);
        crow::connections::systemBus->async_method_call(
            [asyncResp, id, reportPath](const boost::system::error_code& ec) {
            if (ec.value() == EBADR ||
                ec == boost::system::errc::host_unreachable)
            {
                messages::resourceNotFound(asyncResp->res, "MetricReport", id);
                return;
            }
            if (ec)
            {
                BMCWEB_LOG_ERROR("respHandler DBus error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            sdbusplus::asio::getProperty<telemetry::TimestampReadings>(
                *crow::connections::systemBus, telemetry::service, reportPath,
                telemetry::reportInterface, "Readings",
                [asyncResp, id](const boost::system::error_code& ec2,
                                const telemetry::TimestampReadings& ret) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR("respHandler DBus error {}", ec2);
                    messages::internalError(asyncResp->res);
                    return;
                }

                telemetry::fillReport(asyncResp->res.jsonValue, id, ret);
                });
            },
            telemetry::service, reportPath, telemetry::reportInterface,
            "Update");
        });
}
} // namespace redfish
