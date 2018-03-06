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

class AccountService : public Node {
 public:
  template <typename CrowApp>
  AccountService(CrowApp& app)
      : Node(app,
             "/redfish/v1/AccountService/") {
    Node::json["@odata.id"] = "/redfish/v1/AccountService";
    Node::json["@odata.type"] = "#AccountService.v1_1_0.AccountService";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#AccountService.AccountService";
    Node::json["Id"] = "AccountService";
    Node::json["Description"] = "BMC User Accounts";
    Node::json["Name"] = "Account Service";
    Node::json["Status"]["State"] = "Enabled";
    Node::json["Status"]["Health"] = "OK";
    Node::json["Status"]["HealthRollup"] = "OK";
    Node::json["ServiceEnabled"] = true;
    Node::json["MinPasswordLength"] = 1;
    Node::json["MaxPasswordLength"] = 20;
    Node::json["Accounts"]["@odata.id"] = "/redfish/v1/AccountService/Accounts";
    Node::json["Roles"]["@odata.id"] = "/redfish/v1/AccountService/Roles";

    entityPrivileges = {
        {crow::HTTPMethod::GET, {{"ConfigureUsers"}, {"ConfigureManager"}}},
        {crow::HTTPMethod::HEAD, {{"Login"}}},
        {crow::HTTPMethod::PATCH, {{"ConfigureUsers"}}},
        {crow::HTTPMethod::PUT, {{"ConfigureUsers"}}},
        {crow::HTTPMethod::DELETE, {{"ConfigureUsers"}}},
        {crow::HTTPMethod::POST, {{"ConfigureUsers"}}}};
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    res.json_value = Node::json;
    res.end();
  }
};

}  // namespace redfish
