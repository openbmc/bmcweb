/*
// Copyright (c) 2018 Intel Corporation
// Copyright (c) 2018 Ampere Computing LLC
/
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

class Power : public Node
{
  public:
    Power(CrowApp& app) :
        Node((app), "/redfish/v1/Chassis/<str>/Power/", std::string())
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
        "/xyz/openbmc_project/sensors/voltage",
        "/xyz/openbmc_project/sensors/power"};
#endif
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }

#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
        const std::string& chassis_name = params[0];
        // In a one chassis system the only supported name is "chassis"
        if (chassis_name != "chassis")
        {
            messages::resourceNotFound(res, "#Chassis.v1_4_0.Chassis",
                                       chassis_name);
            res.end();
            return;
        }

        res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassis_name + "/Power";
        res.jsonValue["@odata.type"] = "#Power.v1_2_1.Power";
        res.jsonValue["@odata.context"] = "/redfish/v1/$metadata#Power.Power";
        res.jsonValue["Id"] = chassis_name + "_" + "Power";
        res.jsonValue["Name"] = "Power";
        res.end();
        return;
#else
        auto sensorAsyncResp =
            std::make_shared<SensorsAsyncResp>(res, typeList, "Power");
        crow::connections::systemBus->async_method_call(
            [sensorAsyncResp,
             params](const boost::system::error_code ec,
                     const std::vector<std::string>& resourcesList) {
                if (ec)
                {
                    messages::internalError(sensorAsyncResp->res);
                    return;
                }
                std::list<std::string> subAssyList;
                std::string chassisNode;
                const std::string& chassis_name = params[0];
                getChassisElements(resourcesList, chassisNode, subAssyList);
                // TODO Need to retrieve Power Control information.
                if ((chassisNode.empty()) || (chassisNode != chassis_name))
                {
                    messages::resourceNotFound(sensorAsyncResp->res,
                                               "#Chassis.v1_4_0.Chassis",
                                               chassis_name);
                    return;
                }
                sensorAsyncResp->chassisId = chassis_name;
                sensorAsyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Chassis/" + chassis_name + "/Power";

                getChassisData(sensorAsyncResp);
                sensorAsyncResp->res.jsonValue["@odata.type"] =
                    "#Power.v1_2_1.Power";
                sensorAsyncResp->res.jsonValue["@odata.context"] =
                    "/redfish/v1/$metadata#Power.Power";
                sensorAsyncResp->res.jsonValue["Id"] =
                    chassis_name + "_" + "Power";
                sensorAsyncResp->res.jsonValue["Name"] = "Power";
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
        setSensorOverride(res, req, params, typeList, "Power");
    }
}; // namespace redfish

#ifndef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
class SubAssyPower : public Node
{
  public:
    SubAssyPower(CrowApp& app) :
        Node((app), "/redfish/v1/Chassis/<str>/<str>/Power/", std::string(),
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
        "/xyz/openbmc_project/sensors/voltage",
        "/xyz/openbmc_project/sensors/power"};
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 2)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        auto sensorAsyncResp =
            std::make_shared<SensorsAsyncResp>(res, typeList, "Power");
        crow::connections::systemBus->async_method_call(
            [sensorAsyncResp,
             params](const boost::system::error_code ec,
                     const std::vector<std::string>& resourcesList) {
                if (ec)
                {
                    messages::internalError(sensorAsyncResp->res);
                    return;
                }
                std::list<std::string> subAssyList;
                std::string chassisNode;
                getChassisElements(resourcesList, chassisNode, subAssyList);
                const std::string& chassis_name = params[0];
                const std::string& nodeId = params[1];
                if ((chassisNode.empty()) || (chassisNode != chassis_name))
                {
                    messages::resourceNotFound(sensorAsyncResp->res,
                                               "#Chassis.v1_4_0.Chassis",
                                               chassis_name);
                    return;
                }
                for (std::string& subassembly : subAssyList)
                {
                    if (subassembly == nodeId)
                    {
                        sensorAsyncResp->chassisId = chassis_name;
                        sensorAsyncResp->nodeId = nodeId;
                        // TODO Need to retrieve Power Control information.
                        sensorAsyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Chassis/" + chassis_name + "/" +
                            nodeId + "/Power";
                        getChassisData(sensorAsyncResp);
                        sensorAsyncResp->res.jsonValue["@odata.type"] =
                            "#Power.v1_2_1.Power";
                        sensorAsyncResp->res.jsonValue["@odata.context"] =
                            "/redfish/v1/$metadata#Power.Power";
                        sensorAsyncResp->res.jsonValue["Id"] =
                            nodeId + "_" + "Power";
                        sensorAsyncResp->res.jsonValue["Name"] = "Power";
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
        setSensorOverride(res, req, params, typeList, "Power");
    }
};
#endif

} // namespace redfish
