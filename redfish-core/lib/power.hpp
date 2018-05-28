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

namespace redfish {

class Power : public Node {
 public:
  Power(CrowApp& app)
      : Node((app), "/redfish/v1/Chassis/<str>/Power/", std::string()) {
    Node::json["@odata.type"] = "#Power.v1_2_1.Power";
    Node::json["@odata.context"] = "/redfish/v1/$metadata#Power.Power";
    Node::json["Id"] = "Power";
    Node::json["Name"] = "Power";

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureManager"}}}};
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    if (params.size() != 1) {
      res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
      res.end();
      return;
    }
    const std::string& chassis_name = params[0];

    // Add specific Chassis sub-node name: Power
    const std::string& subNodeName = "Power";
    Node::json["@odata.id"] = "/redfish/v1/Chassis/" + chassis_name + "/Power";
    res.json_value = Node::json;
    auto sensorAsyncResp = std::make_shared<SensorAsyncResp>(
        res, chassis_name,
        std::initializer_list<const char*>{
            "/xyz/openbmc_project/sensors/voltage",
            "/xyz/openbmc_project/sensors/power"},
        subNodeName);
    // TODO Need to retrieve Power Control information.
    getChassisData(sensorAsyncResp);
  }
};

}  // namespace redfish
