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

static constexpr const char* metricReportDefinitionUri =
    "/redfish/v1/TelemetryService/MetricReportDefinitions/";
static constexpr const char* metricReportUri =
    "/redfish/v1/TelemetryService/MetricReports/";
static constexpr const char* reportInterface =
    "xyz.openbmc_project.MonitoringService.Report";
static constexpr const char* telemetryPath =
    "/xyz/openbmc_project/MonitoringService/Reports/TelemetryService";

static void getReportCollection(const std::shared_ptr<AsyncResp>& asyncResp,
                                const char* uri)
{
    const std::array<const char*, 1> interfaces = {reportInterface};

    dbus::utility::getSubTreePaths(
        [asyncResp, uri](const boost::system::error_code ec,
                         const std::vector<std::string>& reports) {
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

            json_util::dbusPathsToMembersArray(asyncResp->res, reports, uri);
        },
        telemetryPath, 1, interfaces);
}

template <typename Callback>
static void getReport(const std::shared_ptr<AsyncResp>& asyncResp,
                      const std::string& id, const char* schemaType,
                      const Callback&& callback)
{
    const std::array<const char*, 1> interfaces = {reportInterface};

    dbus::utility::getSubTreePaths(
        [asyncResp, id, schemaType,
         callback](const boost::system::error_code ec,
                   const std::vector<std::string>& reports) {
            if (ec == boost::system::errc::no_such_file_or_directory)
            {
                messages::resourceNotFound(asyncResp->res, schemaType, id);
                return;
            }

            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                return;
            }

            const std::string target = "/xyz/openbmc_project/"
                                       "MonitoringService/Reports/"
                                       "TelemetryService/" +
                                       id;
            auto path = std::find(reports.begin(), reports.end(), target);
            if (path == std::end(reports))
            {
                messages::resourceNotFound(asyncResp->res, schemaType, id);
                return;
            }
            callback(asyncResp, *path, id);
        },
        telemetryPath, 1, interfaces);
}
} // namespace telemetry
} // namespace redfish
