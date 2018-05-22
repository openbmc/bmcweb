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
  void get_all_software_inventory_object(CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code error_code,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
          CROW_LOG_DEBUG << "get all software inventory object callback...";
          if (error_code) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of vector may vary depending on information from Entity Manager,
            // and empty output could not be treated same way as error.
            callback(false, subtree);
            return;
          }

          if (subtree.empty()) {
            CROW_LOG_DEBUG << "subtree empty";
            callback(false, subtree);
          } else {
            CROW_LOG_DEBUG << "subtree has something";
            callback(true, subtree);
          }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/software", int32_t(1),
        std::array<const char *, 1>{"xyz.openbmc_project.Software.Version"});
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
    Node::json["FirmwareInventory"] = {
        {"@odata.id", "/redfish/v1/UpdateService/FirmwareInventory"}};

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
      : Node(app, "/redfish/v1/UpdateService/FirmwareInventory/") {
    Node::json["@odata.type"] =
        "#SoftwareInventoryCollection.SoftwareInventoryCollection";
    Node::json["@odata.id"] = "/redfish/v1/UpdateService/FirmwareInventory";
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
    software_inventory_provider.get_all_software_inventory_object(
        [&](const bool &success,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
          if (!success) {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
          }

          if (subtree.empty()) {
            CROW_LOG_DEBUG << "subtree empty!!";
            res.end();
            return;
          }

          res.json_value["Members"] = nlohmann::json::array();

          for (auto &obj : subtree) {
            const std::vector<std::pair<std::string, std::vector<std::string>>>
                &connections = obj.second;

            for (auto &conn : connections) {
              const std::string connectionName = conn.first;
              CROW_LOG_DEBUG << "connectionName = " << connectionName;
              CROW_LOG_DEBUG << "obj.first = " << obj.first;

              crow::connections::system_bus->async_method_call(
                  [&](const boost::system::error_code error_code,
                      const boost::container::flat_map<std::string, VariantType>
                          &propertiesList) {
                    CROW_LOG_DEBUG << "safe returned in lambda function";
                    if (error_code) {
                      res.result(
                          boost::beast::http::status::internal_server_error);
                      res.end();
                      return;
                    }
                    boost::container::flat_map<std::string,
                                               VariantType>::const_iterator it =
                        propertiesList.find("Purpose");
                    const std::string &sw_inv_purpose =
                        *(mapbox::get_ptr<const std::string>(it->second));
                    std::size_t last_pos = sw_inv_purpose.rfind(".");
                    if (last_pos != std::string::npos) {
                      res.json_value["Members"].push_back(
                          {{"@odata.id",
                            "/redfish/v1/UpdateService/FirmwareInventory/" +
                                sw_inv_purpose.substr(last_pos + 1)}});
                      res.json_value["Members@odata.count"] =
                          res.json_value["Members"].size();
                      res.end();
                    }

                  },
                  connectionName, obj.first, "org.freedesktop.DBus.Properties",
                  "GetAll", "xyz.openbmc_project.Software.Version");
            }
          }
        });
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
      : Node(app, "/redfish/v1/UpdateService/FirmwareInventory/<str>/",
             std::string()) {
    Node::json["@odata.type"] = "#SoftwareInventory.v1_1_0.SoftwareInventory";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#SoftwareInventory.SoftwareInventory";
    Node::json["Name"] = "Software Inventory";
    Node::json["Status"] = "OK";  // TODO
    Node::json["Updateable"] = false;

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

    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.end();
      return;
    }

    const std::string &sw_id = params[0];
    res.json_value["@odata.id"] =
        "/redfish/v1/UpdateService/FirmwareInventory/" + sw_id;
    software_inventory_provider.get_all_software_inventory_object(
        [&, id{std::string(sw_id)} ](
            const bool &success,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
          CROW_LOG_DEBUG << "doGet callback...";
          if (!success) {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
          }

          if (subtree.empty()) {
            CROW_LOG_DEBUG << "subtree empty!!";
            res.end();
            return;
          }

          for (auto &obj : subtree) {
            const std::vector<std::pair<std::string, std::vector<std::string>>>
                &connections = obj.second;

            for (auto &conn : connections) {
              const std::string connectionName = conn.first;
              CROW_LOG_DEBUG << "connectionName = " << connectionName;
              CROW_LOG_DEBUG << "obj.first = " << obj.first;

              crow::connections::system_bus->async_method_call(
                  [&, id{std::string(id)} ](
                      const boost::system::error_code error_code,
                      const boost::container::flat_map<std::string, VariantType>
                          &propertiesList) {
                    if (error_code) {
                      res.result(
                          boost::beast::http::status::internal_server_error);
                      res.end();
                      return;
                    }
                    boost::container::flat_map<std::string,
                                               VariantType>::const_iterator it =
                        propertiesList.find("Purpose");
                    if (it == propertiesList.end()) {
                      CROW_LOG_DEBUG << "Can't find property \"Purpose\"!";
                      return;
                    }
                    const std::string &sw_inv_purpose =
                        *(mapbox::get_ptr<const std::string>(it->second));
                    CROW_LOG_DEBUG << "sw_inv_purpose = " << sw_inv_purpose;
                    if (boost::ends_with(sw_inv_purpose, "." + id)) {
                      it = propertiesList.find("Version");
                      if (it == propertiesList.end()) {
                        CROW_LOG_DEBUG << "Can't find property \"Version\"!";
                        return;
                      }
                      res.json_value["Version"] =
                          *(mapbox::get_ptr<const std::string>(it->second));
                      res.json_value["Id"] = id;
                      res.end();
                    }

                  },
                  connectionName, obj.first, "org.freedesktop.DBus.Properties",
                  "GetAll", "xyz.openbmc_project.Software.Version");
            }
          }
        });
  }

  OnDemandSoftwareInventoryProvider software_inventory_provider;
};

}  // namespace redfish
