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

#include <boost/container/flat_map.hpp>

#include <system_error>
#include <variant>

namespace redfish
{

class MetricReportCollection : public Node
{
  public:
    MetricReportCollection(CrowApp& app) : Node(app, telemetry::metricReportUri)
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
    MetricReport(CrowApp& app) :
        Node(app, std::string(telemetry::metricReportUri) + "<str>/",
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
        telemetry::getReport(asyncResp, id, schemaType, getReportProperties);
    }

    using Readings =
        std::vector<std::tuple<std::string, std::string, double, int32_t>>;
    using MetricValues = std::vector<std::map<std::string, std::string>>;

    static MetricValues toMetricValues(const Readings& readings)
    {
        MetricValues metricValues;

        for (auto& [id, metadata, sensorValue, timestamp] : readings)
        {
            metricValues.push_back({
                {"MetricId", id},
                {"MetricProperty", metadata},
                {"MetricValue", std::to_string(sensorValue)},
                {"Timestamp", crow::utility::getDateTime(timestamp)},
            });
        }

        return metricValues;
    }

    static void getReportProperties(const std::shared_ptr<AsyncResp> asyncResp,
                                    const std::string& reportPath,
                                    const std::string& id)
    {
        asyncResp->res.jsonValue["@odata.type"] = schemaType;
        asyncResp->res.jsonValue["@odata.id"] = telemetry::metricReportUri + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = id;
        asyncResp->res.jsonValue["MetricReportDefinition"]["@odata.id"] =
            telemetry::metricReportDefinitionUri + id;

        dbus::utility::getAllProperties(
            [asyncResp](
                const boost::system::error_code ec,
                const boost::container::flat_map<
                    std::string, std::variant<Readings, int32_t>>& ret) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    return;
                }

                json_util::assignIfPresent<int32_t>(
                    ret, "Timestamp", asyncResp->res.jsonValue["Timestamp"],
                    crow::utility::getDateTime);
                json_util::assignIfPresent<Readings>(
                    ret, "Readings", asyncResp->res.jsonValue["MetricValues"],
                    toMetricValues);
            },
            "xyz.openbmc_project.MonitoringService", reportPath,
            "xyz.openbmc_project.MonitoringService.Report");
    }

    static constexpr const char* schemaType =
        "#MetricReport.v1_3_0.MetricReport";
};
} // namespace redfish
