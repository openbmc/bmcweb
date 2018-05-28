/*
// Copyright (c) 2018 Intel Corporation
// Copyright (c) 2018 Ampere Computing LLC
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
#include <boost/container/flat_map.hpp>

namespace redfish {

/**
 * SystemsCollection derived class for delivering
 * Computer Systems Collection Schema
 */
class SystemsCollection : public Node {
 public:
  template <typename CrowApp>
  SystemsCollection(CrowApp &app): Node(app, "/redfish/v1/Systems/") {
    Node::json["@odata.type"] =
                          "#ComputerSystemCollection.ComputerSystemCollection";
    Node::json["@odata.id"] = "/redfish/v1/Systems";
    Node::json["@odata.context"] =
        "/redfish/v1/"
        "$metadata#ComputerSystemCollection.ComputerSystemCollection";
    Node::json["Name"] = "Computer System Collection";
    Node::json["Members"] =
                    {{{"@odata.id", "/redfish/v1/Systems/1"}}};
    Node::json["Members@odata.count"] = 1; // TODO hardcoded number
                                           // of base board to 1

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureComponents"}}}};
  }

 private:
  /**
   * Functions triggers appropriate requests on D-Bus
   */
  void doGet(crow::response &res, const crow::request &req,
             const std::vector<std::string> &params) override {
    res.json_value = Node::json;
    res.end();
  }

};

/**
* Systems derived class for delivering Computer Systems Schema.
*/
class Systems : public Node {
 public:
  template <typename CrowApp>
  Systems(CrowApp &app)
       : Node(app, "/redfish/v1/Systems/<str>/", std::string()) {
     Node::json["@odata.type"] = "#ComputerSystem.v1_5_0.ComputerSystem";
     Node::json["@odata.context"] =
                  "/redfish/v1/$metadata#ComputerSystem.ComputerSystem";
     Node::json["Name"] = "Computer System";
     Node::json["LogServices"] =
                     {{"@odata.id", "/redfish/v1/Systems/1/LogServices"}};
     Node::json["Links"]["Chassis"] =
                    {{{"@odata.id", "/redfish/v1/Chassis/1"}}};
     Node::json["Links"]["ManagedBy"] =
                    {{{"@odata.id", "/redfish/v1/Managers/bmc"}}};
     Node::json["Id"] = 1; // TODO hardcoded number of base board to 1.

     entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                         {crow::HTTPMethod::HEAD, {{"Login"}}},
                         {crow::HTTPMethod::PATCH, {{"ConfigureComponents"}}},
                         {crow::HTTPMethod::PUT, {{"ConfigureComponents"}}},
                         {crow::HTTPMethod::DELETE, {{"ConfigureComponents"}}},
                         {crow::HTTPMethod::POST, {{"ConfigureComponents"}}}};
   }

 private:
  /**
   * Functions triggers appropriate requests on D-Bus
   */
  void doGet(crow::response &res, const crow::request &req,
              const std::vector<std::string> &params) override {
    // Check if there is required param, truly entering this shall be
    // impossible
    if (params.size() != 1) {
      res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
      res.end();
      return;
    }

    // Get system id
    const std::string &id = params[0];

    res.json_value = Node::json;
    res.json_value["@odata.id"] = "/redfish/v1/Systems/" + id;
    res.end();
  }

};

}  // namespace redfish
