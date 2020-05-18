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
#include <boost/algorithm/string/split.hpp>
#include <boost/container/flat_map.hpp>

#include <regex>
#include <system_error>
#include <tuple>
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
        res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricReportDefinitions";
        res.jsonValue["Name"] = "Metric Definition Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        telemetry::getReportCollection(asyncResp,
                                       telemetry::metricReportDefinitionUri);
    }

    using ChassisSensorNode = std::pair<std::string, std::string>;
    using DbusSensor = sdbusplus::message::object_path;
    using DbusSensors = std::vector<DbusSensor>;
    using MetricParam =
        std::tuple<DbusSensors, std::string, std::string, std::string>;
    using MetricParams = std::vector<MetricParam>;
    /*
     * AddReportArgs misses "Domain" parameter because it is constant for
     * TelemetryService and equals "TelemetryService".
     */
    using AddReportArgs =
        std::tuple<std::string, std::string, std::vector<std::string>, uint32_t,
                   MetricParams>;

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        AddReportArgs addReportArgs;
        if (!getUserParameters(res, req, addReportArgs))
        {
            return;
        }

        boost::container::flat_set<ChassisSensorNode> chassisSensorSet;
        auto unmatched = getChassisSensorNode(
            std::get<MetricParams>(addReportArgs), chassisSensorSet);
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
                    const boost::beast::http::status status,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) { *addReportReq += uriToDbus; });
        }
    }

    static std::optional<std::string>
        replaceReportActions(std::vector<std::string>& actions)
    {
        const boost::container::flat_map<std::string, std::string>
            reportActions = {
                {"RedfishEvent", "Event"},
                {"LogToMetricReportsCollection", "Log"},
            };

        for (auto& action : actions)
        {
            auto found = reportActions.find(action);
            if (found == reportActions.end())
            {
                return action;
            }
            action = found->second;
        }
        return std::nullopt;
    }

    static constexpr const std::array<const char*, 2> supportedDefinitionType =
        {"Periodic", "OnRequest"};

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
            std::make_tuple(std::cref(name), "Id", 255),
            std::make_tuple(std::cref(reportingType),
                            "MetricReportDefinitionType", 255),
            std::make_tuple(std::cref(reportActions), "ReportActions", 255),
            std::make_tuple(metrics.size(), "Metrics", 255));

        if (!validateParamsLength(res, std::move(limits)))
        {
            return false;
        }

        constexpr const char* allowedCharactersInName =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
            "_";
        if (name.empty() || name.find_first_not_of(allowedCharactersInName) !=
                                std::string::npos)
        {
            BMCWEB_LOG_ERROR << "Failed to match " << name
                             << " with allowed character "
                             << allowedCharactersInName;
            messages::propertyValueFormatError(res, name, "Id");
            return false;
        }

        if (!std::any_of(
                supportedDefinitionType.begin(), supportedDefinitionType.end(),
                [reportingType](auto& x) { return reportingType == x; }))
        {
            messages::propertyValueNotInList(res, reportingType,
                                             "MetricReportDefinitionType");
            return false;
        }

        auto unmatched = replaceReportActions(reportActions);
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

            if (!validateParamLength(res, interval, "RecurrenceInterval", 255))
            {
                return false;
            }

            constexpr const char* durationPattern =
                "-?P(\\d+D)?(T(\\d+H)?(\\d+M)?(\\d+(.\\d+)?S)?)?";
            if (!std::regex_match(interval, std::regex(durationPattern)))
            {
                messages::propertyValueFormatError(res, interval,
                                                   "RecurrenceInterval");
                return false;
            }

            scanPeriod = time_utils::fromDurationFormat(interval);
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
                std::make_tuple(std::cref(metricId), "MetricId", 1024),
                std::make_tuple(std::cref(metricProperties), "MetricProperties",
                                1024));

            if (!validateParamsLength(res, std::move(limits)))
            {
                return false;
            }

            DbusSensors dbusSensors;
            dbusSensors.reserve(metricProperties.size());
            std::for_each(
                metricProperties.begin(), metricProperties.end(),
                [&dbusSensors](auto& x) { dbusSensors.emplace_back(x); });

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
                /*
                 * Support only following paths:
                 *   "/redfish/v1/Chassis/<chassis>/Power#/..."
                 *   "/redfish/v1/Chassis/<chassis>/Sensors/..."
                 *   "/redfish/v1/Chassis/<chassis>/Thermal#/..."
                 */
                constexpr const char* uriPattern =
                    "\\/redfish\\/v1\\/Chassis\\/(\\w+)\\/"
                    "(Power|Sensors|Thermal)[#]?\\/.*";
                std::smatch m;
                if (!std::regex_match(sensor.str, m, std::regex(uriPattern)) ||
                    m.size() != 3)
                {
                    BMCWEB_LOG_ERROR << "Failed to match " << sensor.str
                                     << " with pattern " << uriPattern;
                    return sensor;
                }

                BMCWEB_LOG_DEBUG << "Chassis=" << m[1] << ", Type=" << m[2];
                matched.emplace(m[1], m[2]);
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
        constexpr const char* domain = "TelemetryService";
        auto& [name, reportingType, reportActions, scanPeriod, metricParams] =
            args;

        crow::connections::systemBus->async_method_call(
            [asyncResp, name](const boost::system::error_code ec,
                              const std::string ret) {
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
            "xyz.openbmc_project.MonitoringService",
            "/xyz/openbmc_project/MonitoringService/Reports",
            "xyz.openbmc_project.MonitoringService.ReportsManagement",
            "AddReport", name, domain, reportingType, reportActions, scanPeriod,
            metricParams);
    }

    class AddReport
    {
      public:
        AddReport(AddReportArgs& args, std::shared_ptr<AsyncResp>& asyncResp) :
            asyncResp{asyncResp}, addReportArgs{args}
        {}
        ~AddReport()
        {
            auto unmatched = replaceUriWithDbus(
                uriToDbus, std::get<MetricParams>(addReportArgs));
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

    using ReadingParameters =
        std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                               std::string, std::string, std::string>>;
    using Metrics = std::vector<std::map<
        std::string, std::variant<std::string, std::vector<std::string>>>>;

    static Metrics toMetrics(const ReadingParameters& params)
    {
        Metrics metrics;

        for (auto& [sensorPaths, operationType, id, metadata] : params)
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
        asyncResp->res.jsonValue["ReportUpdates"] = "Overwrite";

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

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];
        telemetry::getReport(asyncResp, id, schemaType, deleteReport);
    }

    static void deleteReport(const std::shared_ptr<AsyncResp>& asyncResp,
                             const std::string& path, const std::string& id)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                asyncResp->res.result(boost::beast::http::status::no_content);
            },
            "xyz.openbmc_project.MonitoringService", path,
            "xyz.openbmc_project.Object.Delete", "Delete");
    }

  public:
    static constexpr const char* schemaType =
        "#MetricReportDefinition.v1_3_0.MetricReportDefinition";
};
} // namespace redfish
