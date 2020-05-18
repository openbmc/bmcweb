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
#include "utils/validate_params_length.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/container/flat_map.hpp>

#include <tuple>
#include <variant>

namespace redfish
{

static constexpr size_t maxShortParamLength = 255;
static constexpr size_t maxLongParamLength = 1024;
static constexpr size_t maxDbusNameLength = 255;
static constexpr size_t maxArraySize = 100;
static constexpr size_t maxReportIdLen =
    maxDbusNameLength - std::string_view(telemetry::reportDir).size();

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
        std::optional<std::string> unmatched =
            getChassisSensorNode(addReportArgs.metricParams, chassisSensorSet);
        if (unmatched)
        {
            messages::resourceNotFound(asyncResp->res, "MetricProperties",
                                       *unmatched);
            return;
        }

        auto addReportReq =
            std::make_shared<AddReport>(addReportArgs, asyncResp);
        for (auto& [chassis, sensorType] : chassisSensorSet)
        {
            retrieveUriToDbusMap(
                chassis, sensorType,
                [asyncResp, addReportReq](
                    const boost::beast::http::status,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) { *addReportReq += uriToDbus; });
        }
    }

    static std::optional<std::string>
        replaceReportActions(std::vector<std::string>& actions)
    {
        for (auto& action : actions)
        {
            if (action == "RedfishEvent")
            {
                action = "Event";
                continue;
            }
            if (action == "LogToMetricReportsCollection")
            {
                action = "Log";
                continue;
            }
            return action;
        }
        return std::nullopt;
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

        auto limits = std::make_tuple(
            ItemSizeValidator(name, "Id", maxReportIdLen),
            ItemSizeValidator(reportingType, "MetricReportDefinitionType",
                              maxShortParamLength),
            ItemSizeValidator(reportActions.size(), "ReportActions.size()",
                              maxArraySize),
            ArrayItemsValidator(reportActions, "ReportActions",
                                maxShortParamLength),
            ItemSizeValidator(metrics.size(), "Metrics.size()", maxArraySize));

        if (!validateParamsLength(res, std::move(limits)))
        {
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
            messages::propertyValueFormatError(res, name, "Id");
            return false;
        }

        if (reportingType != "Periodic" && reportingType != "OnRequest")
        {
            messages::propertyValueNotInList(res, reportingType,
                                             "MetricReportDefinitionType");
            return false;
        }

        std::optional<std::string> unmatched =
            replaceReportActions(reportActions);
        if (unmatched)
        {
            messages::propertyValueNotInList(res, *unmatched, "ReportActions");
            return false;
        }

        if (reportingType == "Periodic")
        {
            if (!schedule)
            {
                messages::createFailedMissingReqProperties(res, "Schedule");
                return false;
            }

            std::string interval;
            if (!json_util::readJson(*schedule, res, "RecurrenceInterval",
                                     interval))
            {
                return false;
            }

            if (!validateParamLength(res, interval, "RecurrenceInterval",
                                     maxShortParamLength))
            {
                return false;
            }

            scanPeriod =
                static_cast<uint32_t>(time_utils::fromDurationFormat(interval));
            if (scanPeriod == 0 || scanPeriod == UINT_MAX)
            {
                messages::propertyValueFormatError(res, interval,
                                                   "RecurrenceInterval");
                return false;
            }
        }

        return fillMetricParams(metrics, res, metricParams);
    }

    static bool fillMetricParams(std::vector<nlohmann::json>& metrics,
                                 crow::Response& res,
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

            auto limits = std::make_tuple(
                ItemSizeValidator(metricId, "MetricId", maxLongParamLength),
                ItemSizeValidator(metricProperties.size(),
                                  "MetricProperties.size()", maxArraySize),
                ArrayItemsValidator(metricProperties, "MetricProperties",
                                    maxLongParamLength));

            if (!validateParamsLength(res, std::move(limits)))
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

    static std::optional<std::string> getChassisSensorNode(
        const MetricParams& metricParams,
        boost::container::flat_set<ChassisSensorNode>& matched)
    {
        for (const auto& metricParam : metricParams)
        {
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
                    return sensor;
                }

                if (boost::ends_with(node, "#"))
                {
                    node.pop_back();
                }

                matched.emplace(chassis, node);
            }
        }
        return std::nullopt;
    }

    static std::optional<std::string> replaceUriWithDbus(
        const boost::container::flat_map<std::string, std::string>& uriToDbus,
        MetricParams& metricParams)
    {
        for (auto& metricParam : metricParams)
        {
            auto& dbusSensors = std::get<DbusSensors>(metricParam);
            for (auto& uri : dbusSensors)
            {
                auto dbus = uriToDbus.find(uri);
                if (dbus == uriToDbus.end())
                {
                    BMCWEB_LOG_ERROR << "Failed to find DBus sensor "
                                        "corresponding to URI "
                                     << uri.str;
                    return uri;
                }
                uri = dbus->second;
            }
        }
        return std::nullopt;
    }

    static void addReport(std::shared_ptr<AsyncResp> asyncResp,
                          AddReportArgs args)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp, name = args.name](const boost::system::error_code ec,
                                          const std::string) {
                if (ec == boost::system::errc::file_exists)
                {
                    messages::resourceAlreadyExists(
                        asyncResp->res, "MetricReportDefinition", "Id", name);
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
            "AddReport", args.name, "TelemetryService", args.reportingType,
            args.reportActions, args.scanPeriod, args.metricParams);
    }

    class AddReport
    {
      public:
        AddReport(AddReportArgs& args, std::shared_ptr<AsyncResp>& asyncResp) :
            asyncResp{asyncResp}, addReportArgs{args}
        {}
        ~AddReport()
        {
            std::optional<std::string> unmatched =
                replaceUriWithDbus(uriToDbus, addReportArgs.metricParams);
            if (unmatched)
            {
                messages::resourceNotFound(asyncResp->res, "MetricProperties",
                                           *unmatched);
                return;
            }

            addReport(asyncResp, addReportArgs);
        }

        AddReport& operator+=(
            const boost::container::flat_map<std::string, std::string>& rhs)
        {
            this->uriToDbus.insert(rhs.begin(), rhs.end());
            return *this;
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

        dbus::utility::getAllProperties(
            telemetry::service, telemetry::getDbusReportPath(id),
            telemetry::reportInterface,
            telemetry::handleErrorCode(asyncResp, schemaType, id),
            [asyncResp,
             id](const std::vector<
                 std::pair<std::string, telemetry::ReportProp>>& ret) {
                fillReportDefinition(asyncResp, id, ret);
            });
    }

    static std::vector<std::string>
        toReportActions(const std::vector<std::string>& actions)
    {
        std::vector<std::string> out;
        for (auto& action : actions)
        {
            if (action == "Event")
            {
                out.emplace_back("RedfishEvent");
                continue;
            }
            if (action == "Log")
            {
                out.emplace_back("LogToMetricReportsCollection");
                continue;
            }
            BMCWEB_LOG_ERROR << "Received invalid ReportActions from backend";
        }
        return out;
    }

    static nlohmann::json toMetrics(const telemetry::ReadingParameters& params)
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
        const std::vector<std::pair<std::string, telemetry::ReportProp>>& ret)
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

        const telemetry::ReportAction* reportActions = nullptr;
        const telemetry::ReadingParameters* readingParams = nullptr;
        const telemetry::ReportingType* reportingType = nullptr;
        telemetry::ScanPeriod scanPeriod = 0;

        if (!dbus::utility::readProperties(ret, "ReportAction", reportActions,
                                           "ReadingParameters", readingParams,
                                           "ReportingType", reportingType,
                                           "ScanPeriod", scanPeriod))
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (reportActions)
        {
            asyncResp->res.jsonValue["ReportActions"] =
                toReportActions(*reportActions);
        }

        if (readingParams)
        {
            asyncResp->res.jsonValue["Metrics"] = toMetrics(*readingParams);
        }

        if (reportingType)
        {
            asyncResp->res.jsonValue["MetricReportDefinitionType"] =
                *reportingType;
            if (*reportingType == "Periodic")
            {
                asyncResp->res.jsonValue["Schedule"]["RecurrenceInterval"] =
                    time_utils::toDurationFormat(
                        std::chrono::milliseconds(scanPeriod));
            }
        }
    }

    void doDelete(crow::Response& res, const crow::Request&,
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
            [asyncResp, id](const boost::system::error_code ec) {
                /*
                 * boost::system::errc and std::errc are missing value for
                 * EBADR error that is defined in Linux.
                 */
                if (ec.value() == EBADR)
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

                asyncResp->res.result(boost::beast::http::status::no_content);
            },
            telemetry::service, reportPath, "xyz.openbmc_project.Object.Delete",
            "Delete");
    }

    static constexpr const char* schemaType =
        "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
};
} // namespace redfish
