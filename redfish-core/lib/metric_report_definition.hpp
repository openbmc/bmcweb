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
#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/container/flat_map.hpp>

#include <climits>
#include <tuple>
#include <variant>

namespace redfish
{

class MetricReportDefinitionCollection : public Node
{
  public:
    MetricReportDefinitionCollection(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/")
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
        res.jsonValue["@odata.type"] = "#MetricReportDefinitionCollection."
                                       "MetricReportDefinitionCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReportDefinitions";
        res.jsonValue["Name"] = "Metric Definition Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        telemetry::getReportCollection(asyncResp,
                                       telemetry::metricReportDefinitionUri);
    }

    using ChassisSensorNode = std::pair<std::string, std::string>;
    using DbusSensors = std::vector<sdbusplus::message::object_path>;
    using MetricParams = std::vector<
        std::tuple<DbusSensors, std::string, std::string, std::string>>;

    struct AddReportArgs
    {
        std::string name;
        std::string reportingType;
        std::vector<std::string> reportActions;
        uint32_t scanPeriod = 0;
        MetricParams metricParams;
    };

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        AddReportArgs addReportArgs;
        if (!getUserParameters(res, req, addReportArgs))
        {
            return;
        }

        boost::container::flat_set<ChassisSensorNode> chassisSensorSet;
        if (!getChassisSensorNode(asyncResp, addReportArgs.metricParams,
                                  chassisSensorSet))
        {
            return;
        }

        auto addReportReq =
            std::make_shared<AddReport>(addReportArgs, asyncResp);
        for (const auto& [chassis, sensorType] : chassisSensorSet)
        {
            retrieveUriToDbusMap(
                chassis, sensorType,
                [asyncResp, addReportReq](
                    const boost::beast::http::status status,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) {
                    if (status != boost::beast::http::status::ok)
                    {
                        BMCWEB_LOG_ERROR << "Failed to retrieve URI to dbus "
                                            "sensors map with err "
                                         << static_cast<unsigned>(status);
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    addReportReq->insert(uriToDbus);
                });
        }
    }

    static bool toDbusReportActions(crow::Response& res,
                                    std::vector<std::string>& actions)
    {
        size_t index = 0;
        for (auto& action : actions)
        {
            if (action == "RedfishEvent")
            {
                action = "Event";
            }
            else if (action == "LogToMetricReportsCollection")
            {
                action = "Log";
            }
            else
            {
                messages::propertyValueNotInList(
                    res, action, "ReportActions/" + std::to_string(index));
                return false;
            }
            index++;
        }
        return true;
    }

    static bool getUserParameters(crow::Response& res, const crow::Request& req,
                                  AddReportArgs& params)
    {
        std::vector<nlohmann::json> metrics;
        std::optional<nlohmann::json> schedule;
        auto& [name, reportingType, reportActions, scanPeriod, metricParams] =
            params;
        if (!json_util::readJson(req, res, "Id", name, "Metrics", metrics,
                                 "MetricReportDefinitionType", reportingType,
                                 "ReportActions", reportActions, "Schedule",
                                 schedule))
        {
            return false;
        }

        if (name.size() > NAME_MAX)
        {
            messages::stringValueTooLong(res, name, NAME_MAX);
            return false;
        }

        constexpr const char* allowedCharactersInName =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
        if (name.empty() || name.find_first_not_of(allowedCharactersInName) !=
                                std::string::npos)
        {
            BMCWEB_LOG_ERROR << "Failed to match " << name
                             << " with allowed character "
                             << allowedCharactersInName;
            messages::propertyValueIncorrect(res, "Id", name);
            return false;
        }

        if (reportingType != "Periodic" && reportingType != "OnRequest")
        {
            messages::propertyValueNotInList(res, reportingType,
                                             "MetricReportDefinitionType");
            return false;
        }

        if (!toDbusReportActions(res, reportActions))
        {
            return false;
        }

        if (reportingType == "Periodic")
        {
            if (!schedule)
            {
                messages::createFailedMissingReqProperties(res, "Schedule");
                return false;
            }

            std::string durationStr;
            if (!json_util::readJson(*schedule, res, "RecurrenceInterval",
                                     durationStr))
            {
                return false;
            }

            std::optional<std::chrono::milliseconds> durationNum =
                time_utils::fromDurationString(durationStr);
            if (!durationNum ||
                durationNum->count() <= std::numeric_limits<uint32_t>::min() ||
                durationNum->count() > std::numeric_limits<uint32_t>::max())
            {
                messages::propertyValueIncorrect(res, "RecurrenceInterval",
                                                 durationStr);
                return false;
            }
            scanPeriod = static_cast<uint32_t>(durationNum->count());
        }

        return fillMetricParams(res, metrics, metricParams);
    }

    static bool fillMetricParams(crow::Response& res,
                                 std::vector<nlohmann::json>& metrics,
                                 MetricParams& metricParams)
    {
        metricParams.reserve(metrics.size());
        for (auto& m : metrics)
        {
            std::string metricId;
            std::vector<std::string> metricProperties;
            if (!json_util::readJson(m, res, "MetricId", metricId,
                                     "MetricProperties", metricProperties))
            {
                return false;
            }

            DbusSensors dbusSensors;
            dbusSensors.reserve(metricProperties.size());
            for (auto& prop : metricProperties)
            {
                dbusSensors.emplace_back(prop);
            }

            metricParams.emplace_back(
                dbusSensors, "SINGLE", metricId,
                boost::algorithm::join(metricProperties, ", "));
        }
        return true;
    }

    static bool getChassisSensorNode(
        const std::shared_ptr<AsyncResp>& asyncResp,
        const MetricParams& metricParams,
        boost::container::flat_set<ChassisSensorNode>& matched)
    {
        for (const auto& metricParam : metricParams)
        {
            size_t index = 0;
            const auto& sensors = std::get<DbusSensors>(metricParam);
            for (const auto& sensor : sensors)
            {
                std::string chassis;
                std::string node;

                if (!boost::starts_with(sensor.str, "/redfish/v1/Chassis/") ||
                    !dbus::utility::getNthStringFromPath(sensor, 3, chassis) ||
                    !dbus::utility::getNthStringFromPath(sensor, 4, node))
                {
                    BMCWEB_LOG_ERROR << "Failed to get chassis and sensor Node "
                                        "from "
                                     << sensor.str;
                    messages::propertyValueIncorrect(asyncResp->res, sensor.str,
                                                     "MetricProperties/" +
                                                         std::to_string(index));
                    return false;
                }

                if (boost::ends_with(node, "#"))
                {
                    node.pop_back();
                }

                matched.emplace(chassis, node);
                index++;
            }
        }
        return true;
    }

    class AddReport
    {
      public:
        AddReport(AddReportArgs& args, std::shared_ptr<AsyncResp>& asyncResp) :
            asyncResp{asyncResp}, addReportArgs{args}
        {}
        ~AddReport()
        {
            if (asyncResp->res.result() != boost::beast::http::status::ok)
            {
                return;
            }

            for (auto& metricParam : addReportArgs.metricParams)
            {
                size_t index = 0;
                auto& dbusSensors = std::get<DbusSensors>(metricParam);
                for (auto& uri : dbusSensors)
                {
                    auto dbus = uriToDbus.find(uri);
                    if (dbus == uriToDbus.end())
                    {
                        BMCWEB_LOG_ERROR << "Failed to find DBus sensor "
                                            "corresponding to URI "
                                         << uri.str;
                        messages::resourceNotFound(asyncResp->res,
                                                   "MetricProperties/" +
                                                       std::to_string(index),
                                                   uri.str);
                        return;
                    }
                    uri = dbus->second;
                    index++;
                }
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp = asyncResp, name = addReportArgs.name](
                    const boost::system::error_code ec, const std::string) {
                    if (ec == boost::system::errc::file_exists)
                    {
                        messages::resourceAlreadyExists(
                            asyncResp->res, "MetricReportDefinition", "Id",
                            name);
                        return;
                    }
                    if (ec == boost::system::errc::too_many_files_open)
                    {
                        messages::createLimitReachedForResource(asyncResp->res);
                        return;
                    }
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                        return;
                    }

                    messages::created(asyncResp->res);
                },
                telemetry::service,
                "/xyz/openbmc_project/MonitoringService/Reports",
                "xyz.openbmc_project.MonitoringService.ReportsManagement",
                "AddReport", addReportArgs.name, "TelemetryService",
                addReportArgs.reportingType, addReportArgs.reportActions,
                addReportArgs.scanPeriod, addReportArgs.metricParams);
        }

        void insert(
            const boost::container::flat_map<std::string, std::string>& el)
        {
            uriToDbus.insert(el.begin(), el.end());
        }

      private:
        std::shared_ptr<AsyncResp> asyncResp;
        AddReportArgs addReportArgs;
        boost::container::flat_map<std::string, std::string> uriToDbus{};
    };
};

class MetricReportDefinition : public Node
{
  public:
    MetricReportDefinition(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricReportDefinitions/<str>/",
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
        crow::connections::systemBus->async_method_call(
            [asyncResp,
             id](const boost::system::error_code& ec,
                 const std::vector<std::pair<
                     std::string,
                     std::variant<std::vector<std::string>, ReadingParameters,
                                  std::string, uint32_t>>>& ret) {
                if (ec == boost::system::errc::no_such_file_or_directory)
                {
                    messages::resourceNotFound(asyncResp->res, schemaType, id);
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                fillReportDefinition(asyncResp, id, ret);
            },
            telemetry::service, telemetry::getDbusReportPath(id),
            "org.freedesktop.DBus.Properties", "GetAll",
            telemetry::reportInterface);
    }

    static bool toRedfishReportActions(const std::vector<std::string>& actions,
                                       std::vector<std::string>& out)
    {
        if (actions.size() > 2)
        {
            return false;
        }
        for (auto& action : actions)
        {
            if (action == "Event")
            {
                out.push_back("RedfishEvent");
            }
            else if (action == "Log")
            {
                out.push_back("LogToMetricReportsCollection");
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    using ReadingParameters =
        std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                               std::string, std::string, std::string>>;

    static nlohmann::json toMetrics(const ReadingParameters& params)
    {
        nlohmann::json metrics;

        for (auto& [sensorPaths, operationType, id, metadata] : params)
        {
            metrics.push_back({
                {"MetricId", id},
                {"MetricProperties", {metadata}},
            });
        }

        return metrics;
    }

    static void fillReportDefinition(
        const std::shared_ptr<AsyncResp>& asyncResp, const std::string& id,
        const std::vector<
            std::pair<std::string,
                      std::variant<std::vector<std::string>, ReadingParameters,
                                   std::string, uint32_t>>>& ret)
    {
        asyncResp->res.jsonValue["@odata.type"] = schemaType;
        asyncResp->res.jsonValue["@odata.id"] =
            telemetry::metricReportDefinitionUri + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = id;
        asyncResp->res.jsonValue["MetricReport"]["@odata.id"] =
            telemetry::metricReportUri + id;
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        asyncResp->res.jsonValue["ReportUpdates"] = "Overwrite";

        const std::vector<std::string>* reportActions = nullptr;
        const ReadingParameters* readingParams = nullptr;
        const std::string* reportingType = nullptr;
        const uint32_t* scanPeriod = nullptr;
        for (const auto& [key, var] : ret)
        {
            if (key == "ReportAction")
            {
                reportActions = std::get_if<std::vector<std::string>>(&var);
            }
            else if (key == "ReadingParameters")
            {
                readingParams = std::get_if<ReadingParameters>(&var);
            }
            else if (key == "ReportingType")
            {
                reportingType = std::get_if<std::string>(&var);
            }
            else if (key == "ScanPeriod")
            {
                scanPeriod = std::get_if<uint32_t>(&var);
            }
        }
        if (!reportActions || !readingParams || !reportingType || !scanPeriod)
        {
            BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
            messages::internalError(asyncResp->res);
            return;
        }

        std::vector<std::string> redfishReportActions;
        if (!toRedfishReportActions(*reportActions, redfishReportActions))
        {
            BMCWEB_LOG_ERROR << "Received invalid ReportActions from backend";
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["ReportActions"] = redfishReportActions;
        asyncResp->res.jsonValue["Metrics"] = toMetrics(*readingParams);
        asyncResp->res.jsonValue["MetricReportDefinitionType"] = *reportingType;
        asyncResp->res.jsonValue["Schedule"]["RecurrenceInterval"] =
            time_utils::toDurationString(
                std::chrono::milliseconds(*scanPeriod));
    }

    static constexpr const char* schemaType =
        "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
};
} // namespace redfish
