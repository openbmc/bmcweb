#pragma once

#include "node.hpp"
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
    json["Timestamp"] =
        crow::utility::getDateTime(static_cast<time_t>(timestamp));
    json["MetricValues"] = toMetricValues(readings);
    return true;
}
} // namespace telemetry

class MetricReportCollection : public Node
{
  public:
    MetricReportCollection(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricReports/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] =
            "#MetricReportCollection.MetricReportCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReports";
        res.jsonValue["Name"] = "Metric Report Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        telemetry::getReportCollection(asyncResp, telemetry::metricReportUri);
    }
};

class MetricReport : public Node
{
  public:
    MetricReport(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricReports/<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];
        const std::string reportPath = telemetry::getDbusReportPath(id);
        crow::connections::systemBus->async_method_call(
            [asyncResp, id, reportPath](const boost::system::error_code& ec) {
                if (ec.value() == EBADR ||
                    ec == boost::system::errc::host_unreachable)
                {
                    messages::resourceNotFound(asyncResp->res, "MetricReport",
                                               id);
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp, id](
                        const boost::system::error_code ec,
                        const std::variant<telemetry::TimestampReadings>& ret) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        if (!telemetry::fillReport(asyncResp->res.jsonValue, id,
                                                   ret))
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
    }
};
} // namespace redfish
