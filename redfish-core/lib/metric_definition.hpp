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

namespace redfish
{

namespace chassis
{
template <typename F>
static inline void getChassisNames(F&& callback)
{
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    dbus::utility::getSubTreePaths(
        [callback{std::move(callback)}](const boost::system::error_code ec,
                                        std::vector<std::string>& chassisList) {
            if (ec)
            {
                return;
            }

            std::vector<std::string> chassisNames;
            chassisNames.reserve(chassisList.size());
            for (auto& chassisPath : chassisList)
            {
                auto pos = chassisPath.rfind("/");
                if (pos == std::string::npos)
                {
                    continue;
                }
                chassisNames.push_back(chassisPath.substr(pos + 1));
            }

            callback(chassisNames);
        },
        "/xyz/openbmc_project/inventory", 0, interfaces);
}
} // namespace chassis

class MetricDefinitionCollection : public Node
{
  public:
    MetricDefinitionCollection(CrowApp& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricDefinitions")
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
        res.jsonValue["@odata.type"] = "#MetricDefinitionCollection."
                                       "MetricDefinitionCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricDefinitions";
        res.jsonValue["Name"] = "Metric Definition Collection";
        res.jsonValue["Members"] = nlohmann::json::array();
        res.jsonValue["Members@odata.count"] = sensors::dbus::types.size();

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto collectionReduce = std::make_shared<CollectionGather>(asyncResp);
        chassis::getChassisNames(
            [asyncResp,
             collectionReduce](const std::vector<std::string>& chassisNames) {
                for (auto& chassisName : chassisNames)
                {
                    for (auto& [sensorNode, _] : sensors::dbus::types)
                    {
                        BMCWEB_LOG_INFO << "Chassis: " << chassisName
                                        << " sensor: " << sensorNode;
                        retrieveUriToDbusMap(
                            chassisName, sensorNode.data(),
                            [asyncResp, collectionReduce](
                                const boost::beast::http::status status,
                                const boost::container::flat_map<
                                    std::string, std::string>& uriToDbus) {
                                *collectionReduce += uriToDbus;
                            });
                    }
                }
            });
    }

    class CollectionGather
    {
      public:
        CollectionGather(const std::shared_ptr<AsyncResp>& asyncResp) :
            asyncResp{asyncResp}
        {
            dbusTypes.reserve(sensors::dbus::paths.size());
        }

        ~CollectionGather()
        {
            json_util::dbusPathsToMembersArray(
                asyncResp->res,
                std::vector<std::string>(dbusTypes.begin(), dbusTypes.end()),
                telemetry::metricDefinitionUri);
        }

        CollectionGather& operator+=(
            const boost::container::flat_map<std::string, std::string>& rhs)
        {
            for (auto& [_, dbusSensor] : rhs)
            {
                auto pos = dbusSensor.rfind("/");
                if (pos == std::string::npos)
                {
                    BMCWEB_LOG_ERROR << "Received invalid DBus Sensor Path = "
                                     << dbusSensor;
                    continue;
                }

                this->dbusTypes.insert(dbusSensor.substr(0, pos));
            }
            return *this;
        }

      private:
        const std::shared_ptr<AsyncResp> asyncResp;
        boost::container::flat_set<std::string> dbusTypes;
    };
};

class MetricDefinition : public Node
{
  public:
    MetricDefinition(CrowApp& app) :
        Node(app, std::string(telemetry::metricDefinitionUri) + "<str>/",
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

        size_t sensorIndex = 0;
        for (auto& name : sensors::dbus::names)
        {
            if (name == id)
            {
                break;
            }
            sensorIndex++;
        }
        if (sensorIndex >= sensors::dbus::max)
        {
            messages::resourceNotFound(asyncResp->res, schemaType, id);
            return;
        }

        auto definitionGather =
            std::make_shared<DefinitionGather>(asyncResp, id);
        chassis::getChassisNames(
            [asyncResp, definitionGather,
             sensorIndex](const std::vector<std::string>& chassisNames) {
                for (auto& chassisName : chassisNames)
                {
                    for (auto& [sensorNode, dbusPaths] : sensors::dbus::types)
                    {
                        auto found =
                            std::find(dbusPaths.begin(), dbusPaths.end(),
                                      sensors::dbus::paths[sensorIndex]);
                        if (found == dbusPaths.end())
                        {
                            continue;
                        }

                        retrieveUriToDbusMap(
                            chassisName, sensorNode.data(),
                            [asyncResp, definitionGather](
                                const boost::beast::http::status status,
                                const boost::container::flat_map<
                                    std::string, std::string>& uriToDbus) {
                                *definitionGather += uriToDbus;
                            });
                    }
                }
            });
    }

    class DefinitionGather
    {
      public:
        DefinitionGather(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& id) :
            id(id),
            asyncResp{asyncResp}
        {}
        ~DefinitionGather()
        {
            if (redfishSensors.empty())
            {
                messages::resourceNotFound(asyncResp->res, schemaType, id);
                return;
            }

            asyncResp->res.jsonValue["MetricProperties"] =
                nlohmann::json::array();
            auto& members = asyncResp->res.jsonValue["MetricProperties"];
            for (auto& redfishSensor : redfishSensors)
            {
                members.push_back(redfishSensor);
            }

            asyncResp->res.jsonValue["Id"] = id;
            asyncResp->res.jsonValue["Name"] = id;
            asyncResp->res.jsonValue["@odata.id"] =
                telemetry::metricDefinitionUri + id;
            asyncResp->res.jsonValue["@odata.type"] = schemaType;
            asyncResp->res.jsonValue["MetricDataType"] = "Decimal";
            asyncResp->res.jsonValue["MetricType"] = "Numeric";
            asyncResp->res.jsonValue["Implementation"] = "PhysicalSensor";
            asyncResp->res.jsonValue["IsLinear"] = true;
            asyncResp->res.jsonValue["TimestampAccuracy"] = "PT0.1S";
            auto unit = sensorUnits.find(id);
            if (unit != sensorUnits.end())
            {
                asyncResp->res.jsonValue["Units"] = unit->second;
            }
        }

        DefinitionGather& operator+=(
            const boost::container::flat_map<std::string, std::string>& rhs)
        {
            for (auto& [redfishSensor, dbusSensor] : rhs)
            {
                if (dbusSensor.find(id) != std::string::npos)
                {
                    this->redfishSensors.push_back(redfishSensor);
                }
            }
            return *this;
        }

        const std::string id;

      private:
        const std::shared_ptr<AsyncResp> asyncResp;
        std::vector<std::string> redfishSensors;
        const boost::container::flat_map<std::string, std::string> sensorUnits =
            {{sensors::dbus::names[sensors::dbus::voltage], "V"},
             {sensors::dbus::names[sensors::dbus::power], "W"},
             {sensors::dbus::names[sensors::dbus::current], "A"},
             {sensors::dbus::names[sensors::dbus::fan_tach], "RPM"},
             {sensors::dbus::names[sensors::dbus::temperature], "Cel"},
             {sensors::dbus::names[sensors::dbus::utilization], "%"},
             {sensors::dbus::names[sensors::dbus::fan_pwm], "Duty cycle"}};
    };

    static constexpr const char* schemaType =
        "#MetricDefinition.v1_0_3.MetricDefinition";
};

} // namespace redfish
