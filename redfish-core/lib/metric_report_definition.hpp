#pragma once

//#include "sensors.hpp" 
#include "../include/utils/json_utils.hpp"
#include <boost/algorithm/string.hpp>
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <app_class_decl.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <registries/privilege_registry.hpp>

#include <tuple>
#include <variant>

namespace redfish
{

namespace telemetry
{

using ReadingParameters =
    std::vector<std::tuple<sdbusplus::message::object_path, std::string,
                           std::string, std::string>>;

inline void fillReportDefinition(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& id,
    const std::vector<
        std::pair<std::string, std::variant<std::string, bool, uint64_t,
                                            ReadingParameters>>>& ret)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
    asyncResp->res.jsonValue["@odata.id"] =
        telemetry::metricReportDefinitionUri + id;
    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["Name"] = id;
    asyncResp->res.jsonValue["MetricReport"]["@odata.id"] =
        telemetry::metricReportUri + id;
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["ReportUpdates"] = "Overwrite";

    const bool* emitsReadingsUpdate = nullptr;
    const bool* logToMetricReportsCollection = nullptr;
    const ReadingParameters* readingParams = nullptr;
    const std::string* reportingType = nullptr;
    const uint64_t* interval = nullptr;
    for (const auto& [key, var] : ret)
    {
        if (key == "EmitsReadingsUpdate")
        {
            emitsReadingsUpdate = std::get_if<bool>(&var);
        }
        else if (key == "LogToMetricReportsCollection")
        {
            logToMetricReportsCollection = std::get_if<bool>(&var);
        }
        else if (key == "ReadingParameters")
        {
            readingParams = std::get_if<ReadingParameters>(&var);
        }
        else if (key == "ReportingType")
        {
            reportingType = std::get_if<std::string>(&var);
        }
        else if (key == "Interval")
        {
            interval = std::get_if<uint64_t>(&var);
        }
    }
    if (!emitsReadingsUpdate || !logToMetricReportsCollection ||
        !readingParams || !reportingType || !interval)
    {
        BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
        messages::internalError(asyncResp->res);
        return;
    }

    std::vector<std::string> redfishReportActions;
    redfishReportActions.reserve(2);
    if (*emitsReadingsUpdate)
    {
        redfishReportActions.emplace_back("RedfishEvent");
    }
    if (*logToMetricReportsCollection)
    {
        redfishReportActions.emplace_back("LogToMetricReportsCollection");
    }

    nlohmann::json metrics = nlohmann::json::array();
    for (auto& [sensorPath, operationType, id, metadata] : *readingParams)
    {
        metrics.push_back({
            {"MetricId", id},
            {"MetricProperties", {metadata}},
        });
    }
    asyncResp->res.jsonValue["Metrics"] = metrics;
    asyncResp->res.jsonValue["MetricReportDefinitionType"] = *reportingType;
    asyncResp->res.jsonValue["ReportActions"] = redfishReportActions;
    asyncResp->res.jsonValue["Schedule"]["RecurrenceInterval"] =
        time_utils::toDurationString(std::chrono::milliseconds(*interval));
}

struct AddReportArgs
{
    std::string name;
    std::string reportingType;
    bool emitsReadingsUpdate = false;
    bool logToMetricReportsCollection = false;
    uint64_t interval = 0;
    std::vector<std::pair<std::string, std::vector<std::string>>> metrics;
};

class AddReport
{
  public:
    AddReport(AddReportArgs argsIn,
              const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
    ~AddReport();
    void insert(const boost::container::flat_map<std::string, std::string>& el);

  private:
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    AddReportArgs args;
    boost::container::flat_map<std::string, std::string> uriToDbus{};
};
} // namespace telemetry

// moved to sensor.hpp
//void requestRoutesMetricReportDefinitionCollection(App& app);

void requestRoutesMetricReportDefinition(App& app)
{
    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::getMetricReportDefinition)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, id](
                        const boost::system::error_code ec,
                        const std::vector<std::pair<
                            std::string,
                            std::variant<std::string, bool, uint64_t,
                                         telemetry::ReadingParameters>>>& ret) {
                        if (ec.value() == EBADR ||
                            ec == boost::system::errc::host_unreachable)
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "MetricReportDefinition", id);
                            return;
                        }
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        telemetry::fillReportDefinition(asyncResp, id, ret);
                    },
                    telemetry::service, telemetry::getDbusReportPath(id),
                    "org.freedesktop.DBus.Properties", "GetAll",
                    telemetry::reportInterface);
            });
    BMCWEB_ROUTE(app,
                 "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/")
        .privileges(redfish::privileges::deleteMetricReportDefinitionCollection)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& id)

            {
                const std::string reportPath = telemetry::getDbusReportPath(id);

                crow::connections::systemBus->async_method_call(
                    [asyncResp, id](const boost::system::error_code ec) {
                        /*
                         * boost::system::errc and std::errc are missing value
                         * for EBADR error that is defined in Linux.
                         */
                        if (ec.value() == EBADR)
                        {
                            messages::resourceNotFound(
                                asyncResp->res, "MetricReportDefinition", id);
                            return;
                        }

                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        asyncResp->res.result(
                            boost::beast::http::status::no_content);
                    },
                    telemetry::service, reportPath,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            });
}

} // namespace redfish
