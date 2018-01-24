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

static OperationMap serviceRootOpMap = {
    {crow::HTTPMethod::GET, {{}}},
    {crow::HTTPMethod::HEAD, {{}}},
    {crow::HTTPMethod::PATCH, {{"ConfigureComponents"}}},
    {crow::HTTPMethod::PUT, {{"ConfigureComponents"}}},
    {crow::HTTPMethod::DELETE, {{"ConfigureComponents"}}},
    {crow::HTTPMethod::POST, {{"ConfigureComponents"}}}};

class ServiceRoot : public Node {
 public:
  ServiceRoot(CrowApp& app)
      : Node(app, EntityPrivileges(std::move(serviceRootOpMap)),
             "/redfish/v1/") {
    Node::json["@odata.type"] = "#ServiceRoot.v1_1_1.ServiceRoot";
    Node::json["@odata.id"] = "/redfish/v1/";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#ServiceRoot.ServiceRoot";
    Node::json["Id"] = "RootService";
    Node::json["Name"] = "Root Service";
    Node::json["RedfishVersion"] = "1.1.0";
    Node::json["Links"]["Sessions"] = {
        {"@odata.id", "/redfish/v1/SessionService/Sessions"}};
    Node::json["UUID"] =
        app.template get_middleware<crow::PersistentData::Middleware>()
            .system_uuid;
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    res.json_value = Node::json;
    res.end();
  }
};

}  // namespace redfish
