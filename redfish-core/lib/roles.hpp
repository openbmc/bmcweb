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

static OperationMap roleOpMap = {
    {crow::HTTPMethod::GET, {{"Login"}}},
    {crow::HTTPMethod::HEAD, {{"Login"}}},
    {crow::HTTPMethod::PATCH, {{"ConfigureManager"}}},
    {crow::HTTPMethod::PUT, {{"ConfigureManager"}}},
    {crow::HTTPMethod::DELETE, {{"ConfigureManager"}}},
    {crow::HTTPMethod::POST, {{"ConfigureManager"}}}};

static OperationMap roleCollectionOpMap = {
    {crow::HTTPMethod::GET, {{"Login"}}},
    {crow::HTTPMethod::HEAD, {{"Login"}}},
    {crow::HTTPMethod::PATCH, {{"ConfigureManager"}}},
    {crow::HTTPMethod::PUT, {{"ConfigureManager"}}},
    {crow::HTTPMethod::DELETE, {{"ConfigureManager"}}},
    {crow::HTTPMethod::POST, {{"ConfigureManager"}}}};

class Roles : public Node {
 public:
  template <typename CrowApp>
  Roles(CrowApp& app)
      : Node(app, EntityPrivileges(std::move(roleOpMap)),
             "/redfish/v1/AccountService/Roles/Administrator/") {
    nodeJson["@odata.id"] = "/redfish/v1/AccountService/Roles/Administrator";
    nodeJson["@odata.type"] = "#Role.v1_0_2.Role";
    nodeJson["@odata.context"] = "/redfish/v1/$metadata#Role.Role";
    nodeJson["Id"] = "Administrator";
    nodeJson["Name"] = "User Role";
    nodeJson["Description"] = "Administrator User Role";
    nodeJson["IsPredefined"] = true;
    nodeJson["AssignedPrivileges"] = {"Login", "ConfigureManager",
                                      "ConfigureUsers", "ConfigureSelf",
                                      "ConfigureComponents"};
    nodeJson["OemPrivileges"] = nlohmann::json::array();
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    res.json_value = nodeJson;
    res.end();
  }

  nlohmann::json nodeJson;
};

class RoleCollection : public Node {
 public:
  template <typename CrowApp>
  RoleCollection(CrowApp& app)
      : Node(app, EntityPrivileges(std::move(roleCollectionOpMap)),
             "/redfish/v1/AccountService/Roles/") {
    nodeJson["@odata.id"] = "/redfish/v1/AccountService/Roles";
    nodeJson["@odata.type"] = "#RoleCollection.RoleCollection";
    nodeJson["@odata.context"] =
        "/redfish/v1/$metadata#RoleCollection.RoleCollection";
    nodeJson["Name"] = "Roles Collection";
    nodeJson["Description"] = "BMC User Roles";
    nodeJson["Members@odata.count"] = 1;
    nodeJson["Members"] = {
        {{"@odata.id", "/redfish/v1/AccountService/Roles/Administrator"}}};
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
