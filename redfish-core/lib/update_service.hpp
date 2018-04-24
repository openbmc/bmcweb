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
#include <boost/container/flat_map.hpp>

namespace redfish {

class OnDemandSoftwareInventoryProvider {
 public:
  template <typename CallbackFunc>
  void get_all_software_inventory_data(CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code error_code,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {

          std::vector<boost::container::flat_map<std::string, std::string>>
              output;

          if (error_code) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of vector may vary depending on information from Entity Manager,
            // and empty output could not be treated same way as error.
            callback(false, output);
            return;
          }

          for (auto &obj : subtree) {
            const std::vector<std::pair<std::string, std::vector<std::string>>>
                &connectionNames = obj.second;

            const std::string connectionName = connectionNames[0].first;

            crow::connections::system_bus->async_method_call(
                [&](const boost::system::error_code error_code,
                    const std::vector<std::pair<std::string, VariantType>>
                        &propertiesList) {
                  for (auto &property : propertiesList) {
                    boost::container::flat_map<std::string, std::string>
                        single_sw_item_properties;
                    single_sw_item_properties[property.first] =
                        *(mapbox::get_ptr<const std::string>(property.second));
                    output.emplace_back(single_sw_item_properties);
                  }
                },
                connectionName, obj.first, "org.freedesktop.DBus.Properties",
                "GetAll", "xyz.openbmc_project.Software.Version");
            // Finally make a callback with usefull data
            callback(true, output);
          }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/software", int32_t(0),
        std::array<const char *, 1>{"xyz.openbmc_project.Software.Version"});
  }
  /*
   * Function that retrieves all SoftwareInventory available through
   * Software.BMC.Updater.
   * @param callback a function that shall be called to convert Dbus output into
   * JSON.
   */
  template <typename CallbackFunc>
  void get_software_inventory_list(CallbackFunc &&callback) {
    get_all_software_inventory_data(
        [callback](
            const bool &success,
            const std::vector<
                boost::container::flat_map<std::string, std::string>> &output) {
          std::vector<std::string> sw_inv_list;
          for (auto &i : output) {
            boost::container::flat_map<std::string, std::string>::const_iterator
                p = i.find("Purpose");
            if ((p != i.end())) {
              const std::string &sw_inv_purpose =
                  boost::get<std::string>(p->second);
              std::size_t last_pos = sw_inv_purpose.rfind(".");
              if (last_pos != std::string::npos) {
                // and put it into output vector.
                sw_inv_list.emplace_back(sw_inv_purpose.substr(last_pos + 1));
              }
            }
          }
          callback(true, sw_inv_list);
        });
  };

  template <typename CallbackFunc>
  void get_software_inventory_data(const std::string &res_name,
                                   CallbackFunc &&callback) {
    get_all_software_inventory_data(
        [res_name, callback](
            const bool &success,
            const std::vector<
                boost::container::flat_map<std::string, std::string>> &output) {
          for (auto &i : output) {
            boost::container::flat_map<std::string, std::string>::const_iterator
                p = i.find("Purpose");
            // Find the one with Purpose matching res_name
            if ((p != i.end()) &&
                boost::ends_with(boost::get<std::string>(p->second),
                                 "." + res_name)) {
              callback(true, i);
            }
          }
        });
  }
};

class UpdateService : public Node {
 public:
  UpdateService(CrowApp &app) : Node(app, "/redfish/v1/UpdateService/") {
    Node::json["@odata.type"] = "#UpdateService.v1_2_0.UpdateService";
    Node::json["@odata.id"] = "/redfish/v1/UpdateService";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#UpdateService.UpdateService";
    Node::json["Id"] = "UpdateService";
    Node::json["Description"] = "Service for Software Update";
    Node::json["Name"] = "Update Service";
    Node::json["ServiceEnabled"] = true;  // UpdateService cannot be disabled
    Node::json["SoftwareInventory"] = {
        {"@odata.id", "/redfish/v1/UpdateService/SoftwareInventory"}};

    entityPrivileges = {
        {boost::beast::http::verb::get, {{"Login"}}},
        {boost::beast::http::verb::head, {{"Login"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  void doGet(crow::response &res, const crow::request &req,
             const std::vector<std::string> &params) override {
    res.json_value = Node::json;
    res.end();
  }
};

class SoftwareInventoryCollection : public Node {
 public:
  /*
   * Default Constructor
   */
  template <typename CrowApp>
  SoftwareInventoryCollection(CrowApp &app)
      : Node(app, "/redfish/v1/UpdateService/SoftwareInventory/") {
    Node::json["@odata.type"] =
        "#SoftwareInventoryCollection.SoftwareInventoryCollection";
    Node::json["@odata.id"] = "/redfish/v1/UpdateService/SoftwareInventory";
    Node::json["@odata.context"] =
        "/redfish/v1/"
        "$metadata#SoftwareInventoryCollection.SoftwareInventoryCollection";
    Node::json["Name"] = "Software Inventory Collection";

    entityPrivileges = {
        {boost::beast::http::verb::get, {{"Login"}}},
        {boost::beast::http::verb::head, {{"Login"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  /**
   * Functions triggers appropriate requests on DBus
   */
  void doGet(crow::response &res, const crow::request &req,
             const std::vector<std::string> &params) override {
    res.json_value = Node::json;
    // Get sw inventory list, and call the below callback for JSON preparation
    software_inventory_provider.get_software_inventory_list(
        [&](const bool &success, const std::vector<std::string> &output) {

          if (success) {
            // ... prepare json array with appropriate @odata.id links
            nlohmann::json sw_inventory_array = nlohmann::json::array();
            for (const std::string &sw_item : output) {
              sw_inventory_array.push_back(
                  {{"@odata.id",
                    "/redfish/v1/UpdateService/SoftwareInventory/" + sw_item}});
            }
            // Then attach members, count size and return

            Node::json["Members"] = sw_inventory_array;
            Node::json["Members@odata.count"] = sw_inventory_array.size();
            res.json_value = Node::json;
          } else {
            // ... otherwise, return INTERNALL ERROR
            res.result(boost::beast::http::status::internal_server_error);
          }
          res.end();
        });
    res.end();
  }
  OnDemandSoftwareInventoryProvider software_inventory_provider;
};
/**
 * Chassis override class for delivering Chassis Schema
 */
class SoftwareInventory : public Node {
 public:
  /*
   * Default Constructor
   */
  template <typename CrowApp>
  SoftwareInventory(CrowApp &app)
      : Node(app, "/redfish/v1/UpdateService/SoftwareInventory/<str>/",
             std::string()) {
    Node::json["@odata.type"] = "#SoftwareInventory.v1_1_0.SoftwareInventory";
    Node::json["@odata.id"] = "/redfish/v1/UpdateService/SoftwareInventory";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#SoftwareInventory.SoftwareInventory";
    Node::json["Name"] = "Software Inventory";
    Node::json["Status"] = "OK";  // TODO
    Node::json["Updateable"] = "No";

    entityPrivileges = {
        {boost::beast::http::verb::get, {{"Login"}}},
        {boost::beast::http::verb::head, {{"Login"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  /**
   * Functions triggers appropriate requests on DBus
   */
  void doGet(crow::response &res, const crow::request &req,
             const std::vector<std::string> &params) override {
    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.end();
      return;
    }
    const std::string &sw_id = params[0];
    software_inventory_provider.get_software_inventory_data(
        sw_id, [&, id{std::string(sw_id)} ](
                   const bool &success,
                   const boost::container::flat_map<std::string, std::string>
                       &output) {
          res.json_value = Node::json;
          // If success...
          if (success) {
            // prepare all the schema required fields.
            res.json_value["@odata.id"] =
                "/redfish/v1/UpdateService/SoftwareInventory/" + id;
            // also the one from dbus
            boost::container::flat_map<std::string, std::string>::const_iterator
                it = output.find("Version");
            res.json_value["Version"] = boost::get<std::string>(it->second);

            res.json_value["Id"] = id;
            // prepare respond, and send
          } else {
            res.result(boost::beast::http::status::not_found);
          }
          res.end();
        });
  }

  OnDemandSoftwareInventoryProvider software_inventory_provider;
};

}  // namespace redfish
