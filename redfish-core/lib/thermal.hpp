/*
// Copyright (c) 2018 Intel Corporation
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

namespace redfish
{

class Thermal : public Node
{
  public:
    Thermal(CrowApp& app) :
        Node((app), "/redfish/v1/Chassis/<str>/Thermal/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
#ifndef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Chassis"};
    std::initializer_list<const char*> typeList = {
        "/xyz/openbmc_project/sensors/fan_tach",
        "/xyz/openbmc_project/sensors/temperature",
        "/xyz/openbmc_project/sensors/fan_pwm"};
#endif
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }
#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
        const std::string& chassisName = params[0];
        // In a one chassis system the only supported name is "chassis"
        if (chassisName != "chassis")
        {
            messages::resourceNotFound(res, "#Chassis.v1_4_0.Chassis",
                                       chassisName);
            res.end();
            return;
        }
#else
        auto sensorAsyncResp =
            std::make_shared<SensorsAsyncResp>(res, typeList, "Thermal");
        crow::connections::systemBus->async_method_call(
            [sensorAsyncResp,
             params](const boost::system::error_code ec,
                     const std::vector<std::string>& resourcesList) {
                if (ec)
                {
                    messages::internalError(sensorAsyncResp->res);
                    return;
                }
                std::map<std::string, std::string> subAssyList;
                std::pair<std::string, std::string> chassisNode;
                const std::string& chassisName = params[0];
                getChassisElements(resourcesList, chassisNode, subAssyList);
                if ((chassisNode.first.empty()) ||
                    (chassisNode.first != chassisName))
                {
                    messages::resourceNotFound(sensorAsyncResp->res,
                                               "#Chassis.v1_4_0.Chassis",
                                               chassisName);
                    return;
                }
                // TODO Need to get Chassis Redundancy information.
                sensorAsyncResp->sensorPath = chassisNode.second;
                sensorAsyncResp->chassisId = chassisName;
                getChassisData(sensorAsyncResp);
                sensorAsyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassisName + "/Thermal";
                sensorAsyncResp->res.jsonValue["@odata.type"] =
                    "#Thermal.v1_4_0.Thermal";

                sensorAsyncResp->res.jsonValue["@odata.context"] =
                    "/redfish/v1/$metadata#Thermal.Thermal";
                sensorAsyncResp->res.jsonValue["Id"] =
                    chassisName + "_" + "Thermal";
                sensorAsyncResp->res.jsonValue["Name"] = "Thermal";
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(0), interfaces);
#endif
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        setSensorOverride(res, req, params, typeList, "Thermal");
    }
};

#ifndef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
class SubAssyThermal : public Node
{
  public:
    SubAssyThermal(CrowApp& app) :
        Node((app), "/redfish/v1/Chassis/<str>/<str>/Thermal/", std::string(),
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    const std::array<const char*, 3> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis",
        "xyz.openbmc_project.Inventory.Item.PowerSupply"};
    std::initializer_list<const char*> typeList = {
        "/xyz/openbmc_project/sensors/fan_tach",
        "/xyz/openbmc_project/sensors/temperature",
        "/xyz/openbmc_project/sensors/fan_pwm"};
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 2)
        {
            messages::internalError(res);
            res.end();
            return;
        }
        auto sensorAsyncResp =
            std::make_shared<SensorsAsyncResp>(res, typeList, "Thermal");
        crow::connections::systemBus->async_method_call(
            [sensorAsyncResp,
             params](const boost::system::error_code ec,
                     const std::vector<std::string>& resourcesList) {
                if (ec)
                {
                    messages::internalError(sensorAsyncResp->res);
                    return;
                }
                std::map<std::string, std::string> subAssyList{};
                std::pair<std::string, std::string> chassisNode{};
                const std::string& chassisName = params[0];
                std::string nodeId = params[1];
                getChassisElements(resourcesList, chassisNode, subAssyList);
                if ((chassisNode.first.empty()) ||
                    (chassisNode.first != chassisName))
                {
                    messages::resourceNotFound(sensorAsyncResp->res,
                                               "#Chassis.v1_4_0.Chassis",
                                               chassisName);
                    return;
                }
                for (auto& subassembly : subAssyList)
                {
                    if (subassembly.first == nodeId)
                    {
                        // TODO Need to get Chassis Redundancy information.
                        sensorAsyncResp->sensorPath = subassembly.second;
                        sensorAsyncResp->chassisId = chassisName;
                        sensorAsyncResp->nodeId = nodeId;
                        getChassisData(sensorAsyncResp);
                        sensorAsyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Chassis/" + chassisName + "/" +
                            nodeId + "/Thermal";
                        sensorAsyncResp->res.jsonValue["@odata.type"] =
                            "#Thermal.v1_4_0.Thermal";
                        sensorAsyncResp->res.jsonValue["@odata.context"] =
                            "/redfish/v1/$metadata#Thermal.Thermal";
                        sensorAsyncResp->res.jsonValue["Id"] =
                            nodeId + "_" + "Thermal";
                        sensorAsyncResp->res.jsonValue["Name"] = "Thermal";
                        return;
                    }
                }
                messages::resourceNotFound(sensorAsyncResp->res,
                                           "#Chassis.v1_4_0.Chassis", nodeId);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(0), interfaces);
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        setSensorOverride(res, req, params, typeList, "Thermal");
    }
};
#endif

} // namespace redfish
