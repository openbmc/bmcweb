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

namespace redfish
{

namespace telemetry
{

constexpr const char* service = "xyz.openbmc_project.MonitoringService";
constexpr const char* reportInterface =
    "xyz.openbmc_project.MonitoringService.Report";
constexpr const char* metricDefinitionUri =
    "/redfish/v1/TelemetryService/MetricDefinitions/";
constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions/";
constexpr const char* metricReportUri =
    "/redfish/v1/TelemetryService/MetricReports/";
constexpr const char* reportDir =
    "/xyz/openbmc_project/MonitoringService/Reports/TelemetryService/";

void dbusPathsToMembers(const std::shared_ptr<AsyncResp>& asyncResp,
                        const std::vector<std::string> dbusPaths,
                        const std::string& uri)
{
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();

    for (const std::string& path : dbusPaths)
    {
        std::size_t pos = path.rfind("/");
        if (pos == std::string::npos)
        {
            BMCWEB_LOG_ERROR << "Failed to find '/' in " << path;
            continue;
        }

        if (path.size() <= (pos + 1))
        {
            BMCWEB_LOG_ERROR << "Failed to parse path " << path;
            continue;
        }

        members.push_back({{"@odata.id", uri + path.substr(pos + 1)}});
    }

    asyncResp->res.jsonValue["Members@odata.count"] = members.size();
}

void getReportCollection(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& uri)
{
    const std::array<const char*, 1> interfaces = {reportInterface};

    crow::connections::systemBus->async_method_call(
        [asyncResp, uri](const boost::system::error_code ec,
                         const std::vector<std::string>& reportPaths) {
            if (ec == boost::system::errc::no_such_file_or_directory)
            {
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;
                return;
            }

            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                return;
            }

            dbusPathsToMembers(asyncResp, reportPaths, uri);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/MonitoringService/Reports/TelemetryService", 1,
        interfaces);
}

std::string getDbusReportPath(const std::string& id)
{
    std::string path = reportDir + id;
    dbus::utility::escapePathForDbus(path);
    return path;
}

} // namespace telemetry
} // namespace redfish
