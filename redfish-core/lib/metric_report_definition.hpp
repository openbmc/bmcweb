/*
// Copyright (c) 2018-2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once

#include "node.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/container/flat_map.hpp>
#include <system_error>
#include <variant>

namespace redfish
{

class MetricReportDefinitionCollection : public Node
{
  public:
    MetricReportDefinitionCollection(CrowApp& app) :
        Node(app, telemetry::metricReportDefinitionUri)
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue["@odata.type"] = "#MetricReportDefinitionCollection."
                                       "MetricReportDefinitionCollection";
        res.jsonValue["@odata.id"] = telemetry::metricReportDefinitionUri;
        res.jsonValue["Name"] = "Metric Definition Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        telemetry::getReportCollection(asyncResp,
                                       telemetry::metricReportDefinitionUri);
    }
};

class MetricReportDefinition : public Node
{
  public:
    MetricReportDefinition(CrowApp& app) :
        Node(app, std::string(telemetry::metricReportDefinitionUri) + "<str>/",
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
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];

        telemetry::getReport(asyncResp, id, schemaType,
                             getReportDefinitonProperties);
    }

    static std::vector<std::string>
        toReportActions(const std::vector<std::string>& actions)
    {
        const boost::container::flat_map<std::string, std::string>
            reportActions = {
                {"Event", "RedfishEvent"},
                {"Log", "LogToMetricReportsCollection"},
            };

        std::vector<std::string> out;
        for (auto& action : actions)
        {
            auto found = reportActions.find(action);
            if (found != reportActions.end())
            {
                out.emplace_back(found->second);
            }
        }
        return out;
    }

    using ReadingParameters = std::vector<
        std::tuple<std::string, std::vector<sdbusplus::message::object_path>,
                   std::string, std::string>>;
    using Metrics = std::vector<std::map<
        std::string, std::variant<std::string, std::vector<std::string>>>>;

    static Metrics toMetrics(const ReadingParameters& params)
    {
        Metrics metrics;

        for (auto& [id, sensorPaths, operationType, metadata] : params)
        {
            metrics.push_back({
                {"MetricId", id},
                {"MetricProperties", std::vector<std::string>() = {metadata}},
            });
        }

        return metrics;
    }

    static void
        getReportDefinitonProperties(const std::shared_ptr<AsyncResp> asyncResp,
                                     const std::string& reportPath,
                                     const std::string& id)
    {
        asyncResp->res.jsonValue["@odata.type"] = schemaType;
        asyncResp->res.jsonValue["@odata.id"] =
            telemetry::metricReportDefinitionUri + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = id;
        asyncResp->res.jsonValue["MetricReport"]["@odata.id"] =
            telemetry::metricReportUri + id;
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

        dbus::utility::getAllProperties(
            [asyncResp](const boost::system::error_code ec,
                        const boost::container::flat_map<
                            std::string,
                            std::variant<std::string, std::vector<std::string>,
                                         uint32_t, ReadingParameters>>& ret) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    return;
                }

                json_util::assignIfPresent<std::vector<std::string>>(
                    ret, "ReportAction",
                    asyncResp->res.jsonValue["ReportActions"], toReportActions);
                auto assigned = json_util::assignIfPresent<std::string>(
                    ret, "ReportingType",
                    asyncResp->res.jsonValue["MetricReportDefinitionType"]);
                if (assigned &&
                    asyncResp->res.jsonValue["MetricReportDefinitionType"] ==
                        "Periodic")
                {
                    json_util::assignIfPresent<uint32_t>(
                        ret, "ScanPeriod",
                        asyncResp->res
                            .jsonValue["Schedule"]["RecurrenceInterval"],
                        time_utils::toDurationFormat);
                }
                json_util::assignIfPresent<ReadingParameters>(
                    ret, "ReadingParameters",
                    asyncResp->res.jsonValue["Metrics"], toMetrics);
            },
            "xyz.openbmc_project.MonitoringService", reportPath,
            "xyz.openbmc_project.MonitoringService.Report");
    }

  public:
    static constexpr const char* schemaType =
        "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
};
} // namespace redfish
