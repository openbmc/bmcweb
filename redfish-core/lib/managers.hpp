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

class Manager : public Node {
 public:
  Manager(CrowApp& app) : Node(app, "/redfish/v1/Managers/openbmc/") {
    Node::json["@odata.id"] = "/redfish/v1/Managers/openbmc";
    Node::json["@odata.type"] = "#Manager.v1_3_0.Manager";
    Node::json["@odata.context"] = "/redfish/v1/$metadata#Manager.Manager";
    Node::json["Id"] = "openbmc";
    Node::json["Name"] = "OpenBmc Manager";
    Node::json["Description"] = "Baseboard Management Controller";
    Node::json["PowerState"] = "On";
    Node::json["UUID"] =
        app.template getMiddleware<crow::persistent_data::Middleware>()
            .systemUuid;
    Node::json["Model"] = "OpenBmc";               // TODO(ed), get model
    Node::json["FirmwareVersion"] = "1234456789";  // TODO(ed), get fwversion
    Node::json["EthernetInterfaces"] = nlohmann::json(
        {{"@odata.id",
          "/redfish/v1/Managers/openbmc/EthernetInterfaces"}});  // TODO(Pawel),
                                                                 // remove this
                                                                 // when
                                                                 // subroutes
                                                                 // will work
                                                                 // correctly

    entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}},
                        {boost::beast::http::verb::head, {{"Login"}}},
                        {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
                        {boost::beast::http::verb::put, {{"ConfigureManager"}}},
                        {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
                        {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
  }

 private:
  void doGet(crow::Response& res, const crow::Request& req,
             const std::vector<std::string>& params) override {
    Node::json["DateTime"] = getDateTime();
    // Copy over the static data to include the entries added by SubRoute
    res.jsonValue = Node::json;
    res.end();
  }

  std::string getDateTime() const {
    std::array<char, 128> dateTime;
    std::string redfishDateTime("0000-00-00T00:00:00Z00:00");
    std::time_t time = std::time(nullptr);

    if (std::strftime(dateTime.begin(), dateTime.size(), "%FT%T%z",
                      std::localtime(&time))) {
      // insert the colon required by the ISO 8601 standard
      redfishDateTime = std::string(dateTime.data());
      redfishDateTime.insert(redfishDateTime.end() - 2, ':');
    }

    return redfishDateTime;
  }
};

class ManagerCollection : public Node {
 public:
  ManagerCollection(CrowApp& app) : Node(app, "/redfish/v1/Managers/") {
    Node::json["@odata.id"] = "/redfish/v1/Managers";
    Node::json["@odata.type"] = "#ManagerCollection.ManagerCollection";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#ManagerCollection.ManagerCollection";
    Node::json["Name"] = "Manager Collection";
    Node::json["Members@odata.count"] = 1;
    Node::json["Members"] = {{{"@odata.id", "/redfish/v1/Managers/openbmc"}}};

    entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}},
                        {boost::beast::http::verb::head, {{"Login"}}},
                        {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
                        {boost::beast::http::verb::put, {{"ConfigureManager"}}},
                        {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
                        {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
  }

 private:
  void doGet(crow::Response& res, const crow::Request& req,
             const std::vector<std::string>& params) override {
    // Collections don't include the static data added by SubRoute because it
    // has a duplicate entry for members
    res.jsonValue["@odata.id"] = "/redfish/v1/Managers";
    res.jsonValue["@odata.type"] = "#ManagerCollection.ManagerCollection";
    res.jsonValue["@odata.context"] =
        "/redfish/v1/$metadata#ManagerCollection.ManagerCollection";
    res.jsonValue["Name"] = "Manager Collection";
    res.jsonValue["Members@odata.count"] = 1;
    res.jsonValue["Members"] = {
        {{"@odata.id", "/redfish/v1/Managers/openbmc"}}};
    res.end();
  }
};

}  // namespace redfish
