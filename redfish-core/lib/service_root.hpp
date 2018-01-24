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

class ServiceRoot : public Node {
 public:
  template <typename CrowApp, typename PrivilegeProvider>
  ServiceRoot(CrowApp& app, PrivilegeProvider& provider)
      : Node(app, provider, "#ServiceRoot.v1_1_1.ServiceRoot", "/redfish/v1/") {
    nodeJson["@odata.type"] = Node::odataType;
    nodeJson["@odata.id"] = Node::odataId;
    nodeJson["@odata.context"] =
        "/redfish/v1/$metadata#ServiceRoot.ServiceRoot";
    nodeJson["Id"] = "RootService";
    nodeJson["Name"] = "Root Service";
    nodeJson["RedfishVersion"] = "1.1.0";
    nodeJson["Links"]["Sessions"] = {
        {"@odata.id", "/redfish/v1/SessionService/Sessions/"}};
    nodeJson["UUID"] =
        app.template get_middleware<crow::PersistentData::Middleware>()
            .system_uuid;
    getRedfishSubRoutes(app, "/redfish/v1/", nodeJson);
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    res.add_header("Content-Type", "application/json");
    res.body = nodeJson.dump();
    res.end();
  }

  nlohmann::json nodeJson;
};

}  // namespace redfish
