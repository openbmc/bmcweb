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

namespace redfish
{

class MetricReportCollection : public Node
{
  public:
    MetricReportCollection(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricReports/")
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
    MetricReport(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricReports/<str>/",
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
        const std::string reportPath = telemetry::getDbusReportPath(id);
        updateReportIfRequired(
            asyncResp, reportPath, [asyncResp, id, reportPath]() {
                crow::connections::systemBus->async_method_call(
                    [asyncResp,
                     id](const boost::system::error_code ec,
                         const std::vector<std::pair<
                             std::string, std::variant<int32_t, Readings,
                                                       ReadingParameters>>>&
                             ret) {
                        if (ec ==
                            boost::system::errc::no_such_file_or_directory)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       schemaType, id);
                            return;
                        }
                        else if (ec)
                        {
                            BMCWEB_LOG_ERROR << "respHandler DBus error " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        fillReport(asyncResp, id, ret);
                    },
                    telemetry::service, telemetry::getDbusReportPath(id),
                    "org.freedesktop.DBus.Properties", "GetAll",
                    telemetry::reportInterface);
            });
    }

    using ReadingParameters =
        std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                               std::string, std::string, std::string>>;
    using Readings =
        std::vector<std::tuple<std::string, std::string, double, int32_t>>;

    static nlohmann::json toMetricValues(const Readings& readings)
    {
        nlohmann::json metricValues;

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

    static void addMetricDefinition(nlohmann::json& metrics,
                                    const ReadingParameters& params)
    {
        for (auto& metric : metrics)
        {
            auto idIt = metric.find("MetricId");
            if (idIt == metric.end() || !idIt->is_string())
            {
                continue;
            }

            auto idPtr = idIt->get_ptr<std::string*>();
            if (!idPtr)
            {
                continue;
            }

            auto param = std::find_if(
                params.begin(), params.end(), [idPtr](const auto& x) {
                    auto& [sensors, type, metricId, metadata] = x;
                    return *idPtr == metricId;
                });
            if (param == params.end())
            {
                continue;
            }

            auto& dbusPaths =
                std::get<std::vector<sdbusplus::message::object_path>>(*param);
            if (dbusPaths.size() != 1)
            {
                continue;
            }

            auto dbusPath = dbusPaths.begin();
            for (size_t i = 0; i < sensors::dbus::paths.size(); i++)
            {
                if (dbusPath->str.find(sensors::dbus::paths[i]) ==
                    std::string::npos)
                {
                    continue;
                }
                metric["MetricDefinition"]["@odata.id"] =
                    telemetry::metricDefinitionUri +
                    std::string(sensors::dbus::names[i]);
                break;
            }
        }
    }

    static void fillReport(
        const std::shared_ptr<AsyncResp>& asyncResp, const std::string& id,
        const std::vector<std::pair<
            std::string, std::variant<int32_t, Readings, ReadingParameters>>>&
            ret)
    {
        asyncResp->res.jsonValue["@odata.type"] = schemaType;
        asyncResp->res.jsonValue["@odata.id"] = telemetry::metricReportUri + id;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = id;
        asyncResp->res.jsonValue["MetricReportDefinition"]["@odata.id"] =
            telemetry::metricReportDefinitionUri + id;

        const int32_t* timestamp = nullptr;
        const Readings* readings = nullptr;
        const ReadingParameters* readingParams = nullptr;
        for (const auto& [key, var] : ret)
        {
            if (key == "Timestamp")
            {
                timestamp = std::get_if<int32_t>(&var);
            }
            else if (key == "Readings")
            {
                readings = std::get_if<Readings>(&var);
            }
            else if (key == "ReadingParameters")
            {
                readingParams = std::get_if<ReadingParameters>(&var);
            }
        }
        if (!timestamp || !readings || !readingParams)
        {
            BMCWEB_LOG_ERROR << "Property type mismatch or property is missing";
            messages::internalError(asyncResp->res);
            return;
        }

        asyncResp->res.jsonValue["Timestamp"] =
            crow::utility::getDateTime(*timestamp);
        asyncResp->res.jsonValue["MetricValues"] = toMetricValues(*readings);
        addMetricDefinition(asyncResp->res.jsonValue["MetricValues"],
                            *readingParams);
    }

    template <typename Callback>
    static void updateReport(Callback&& callback,
                             const std::shared_ptr<AsyncResp>& asyncResp,
                             const std::string& path)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp, callback = std::move(callback)](
                const boost::system::error_code& ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                callback();
            },
            telemetry::service, path, telemetry::reportInterface, "Update");
    }

    template <typename Callback>
    static void
        updateReportIfRequired(const std::shared_ptr<AsyncResp> asyncResp,
                               const std::string& reportPath,
                               Callback&& callback)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp, reportPath, callback = std::move(callback)](
                const boost::system::error_code& ec,
                const std::variant<std::string>& val) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                auto reportingType = std::get_if<std::string>(&val);
                if (!reportingType)
                {
                    messages::internalError(asyncResp->res);
                }

                if (*reportingType == "OnRequest")
                {
                    updateReport(callback, asyncResp, reportPath);
                }
                else
                {
                    callback();
                }
            },
            telemetry::service, reportPath, "org.freedesktop.DBus.Properties",
            "Get", telemetry::reportInterface, "ReportingType");
    }

    static constexpr const char* schemaType =
        "#MetricReport.v1_3_0.MetricReport";
};
} // namespace redfish
