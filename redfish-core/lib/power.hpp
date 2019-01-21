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
    std::initializer_list<const char*> typeList = {
        "/xyz/openbmc_project/sensors/voltage",
        "/xyz/openbmc_project/sensors/power"};
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
        const std::string& chassis_name = params[0];
#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
        // In a one chassis system the only supported name is "chassis"
        if (chassis_name != "chassis")
        {
            messages::resourceNotFound(res, "#Chassis.v1_4_0.Chassis",
                                       chassis_name);
            res.end();
            return;
        }
#endif

        res.jsonValue["@odata.id"] =
            "/redfish/v1/Chassis/" + chassis_name + "/Power";
        res.jsonValue["@odata.type"] = "#Power.v1_2_1.Power";
        res.jsonValue["@odata.context"] = "/redfish/v1/$metadata#Power.Power";
        res.jsonValue["Id"] = "Power";
        res.jsonValue["Name"] = "Power";
#ifdef BMCWEB_ENABLE_REDFISH_ONE_CHASSIS
        res.end();
        return;
#endif
        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            res, chassis_name, typeList, "Power");
        // TODO Need to retrieve Power Control information.
        getChassisData(sensorAsyncResp);
    }
    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        setSensorOverride(res, req, params, typeList, "Power");
    }
};

} // namespace redfish
