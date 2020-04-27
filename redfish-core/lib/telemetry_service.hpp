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

#include <variant>

namespace redfish
{

class TelemetryService : public Node
{
  public:
    TelemetryService(App& app) : Node(app, "/redfish/v1/TelemetryService/")
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

        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](
                const boost::system::error_code ec,
                const std::vector<
                    std::pair<std::string, std::variant<uint32_t>>>& ret) {
                if (ec == boost::system::errc::host_unreachable)
                {
                    asyncResp->res.jsonValue["Status"]["State"] = "Absent";
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

                const uint32_t* maxReports = nullptr;
                const uint32_t* resolution = nullptr;
                for (const auto& [key, var] : ret)
                {
                    if (key == "MaxReports")
                    {
                        maxReports = std::get_if<uint32_t>(&var);
                    }
                    else if (key == "PollRateResolution")
                    {
                        resolution = std::get_if<uint32_t>(&var);
                    }
                }
                if (!maxReports || !resolution)
                {
                    BMCWEB_LOG_ERROR
                        << "Property type mismatch or property is missing";
                    messages::internalError(asyncResp->res);
                    return;
                }

                asyncResp->res.jsonValue["MaxReports"] = *maxReports;
                asyncResp->res.jsonValue["MinCollectionInterval"] =
                    time_utils::toDurationString(
                        std::chrono::milliseconds(*resolution));
            },
            telemetry::service,
            "/xyz/openbmc_project/MonitoringService/Reports",
            "org.freedesktop.DBus.Properties", "GetAll",
            "xyz.openbmc_project.MonitoringService.ReportsManagement");
    }
};
} // namespace redfish
