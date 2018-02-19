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

namespace redfish {

static OperationMap accountServiceOpMap = {
    {crow::HTTPMethod::GET, {{"ConfigureUsers"}, {"ConfigureManager"}}},
    {crow::HTTPMethod::HEAD, {{"Login"}}},
    {crow::HTTPMethod::PATCH, {{"ConfigureUsers"}}},
    {crow::HTTPMethod::PUT, {{"ConfigureUsers"}}},
    {crow::HTTPMethod::DELETE, {{"ConfigureUsers"}}},
    {crow::HTTPMethod::POST, {{"ConfigureUsers"}}}};

class AccountService : public Node {
 public:
  template <typename CrowApp>
  AccountService(CrowApp& app)
      : Node(app, EntityPrivileges(std::move(accountServiceOpMap)),
             "/redfish/v1/AccountService/") {
    nodeJson["@odata.id"] = "/redfish/v1/AccountService";
    nodeJson["@odata.type"] = "#AccountService.v1_1_0.AccountService";
    nodeJson["@odata.context"] =
        "/redfish/v1/$metadata#AccountService.AccountService";
    nodeJson["Id"] = "AccountService";
    nodeJson["Description"] = "BMC User Accounts";
    nodeJson["Name"] = "Account Service";
    nodeJson["Status"]["State"] = "Enabled";
    nodeJson["Status"]["Health"] = "OK";
    nodeJson["Status"]["HealthRollup"] = "OK";
    nodeJson["ServiceEnabled"] = true;
    nodeJson["MinPasswordLength"] = 1;
    nodeJson["MaxPasswordLength"] = 20;
    nodeJson["Accounts"]["@odata.id"] = "/redfish/v1/AccountService/Accounts";
    nodeJson["Roles"]["@odata.id"] = "/redfish/v1/AccountService/Roles";
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    res.json_value = nodeJson;
    res.end();
  }

  nlohmann::json nodeJson;
};

}  // namespace redfish
