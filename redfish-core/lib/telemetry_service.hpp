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
#include "utils/time_utils.hpp"

#include <boost/container/flat_map.hpp>
#include <variant>

namespace redfish
{

class TelemetryService : public Node
{
  public:
    TelemetryService(CrowApp& app) : Node(app, "/redfish/v1/TelemetryService/")
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
            "#TelemetryService.v1_2_0.TelemetryService";
        res.jsonValue["@odata.id"] = "/redfish/v1/TelemetryService";
        res.jsonValue["Id"] = "TelemetryService";
        res.jsonValue["Name"] = "Telemetry Service";

        res.jsonValue["LogService"]["@odata.id"] =
            "/redfish/v1/Managers/bmc/LogServices/Journal";
        res.jsonValue["MetricReportDefinitions"]["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReportDefinitions";
        res.jsonValue["MetricReports"]["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReports";

        getMonitoringServiceProperties(res);
    }

    void getMonitoringServiceProperties(crow::Response& res)
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        dbus::utility::getAllProperties(
            [asyncResp](
                const boost::system::error_code ec,
                const boost::container::flat_map<std::string,
                                                 std::variant<uint32_t>>& ret) {
                if (ec)
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Absent";
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    return;
                }

                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                json_util::assignIfPresent<uint32_t>(ret, "MaxReports",
                                                     asyncResp->res);
                json_util::assignIfPresent<uint32_t>(
                    ret, "PollRateResolution",
                    asyncResp->res.jsonValue["MinCollectionInterval"],
                    time_utils::toDurationFormat);
            },
            "xyz.openbmc_project.MonitoringService",
            "/xyz/openbmc_project/MonitoringService/Reports",
            "xyz.openbmc_project.MonitoringService.ReportsManagement");
    }
};
} // namespace redfish
