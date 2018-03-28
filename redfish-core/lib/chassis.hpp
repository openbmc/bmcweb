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

/**
 * DBus types primitives for several generic DBus interfaces
 * TODO(Pawel) consider move this to separate file into boost::dbus
 */
// Note, this is not a very useful variant, but because it isn't used to get
// values, it should be as simple as possible
// TODO(ed) invent a nullvariant type
using VariantType = sdbusplus::message::variant<std::string>;
using ManagedObjectsType = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<std::string,
                          std::vector<std::pair<std::string, VariantType>>>>>>;

using PropertiesType = boost::container::flat_map<std::string, VariantType>;

/**
 * OnDemandChassisProvider
 * Chassis provider class that retrieves data directly from dbus, before seting
 * it into JSON output. This does not cache any data.
 *
 * Class can be a good example on how to scale different data providing
 * solutions to produce single schema output.
 *
 * TODO(Pawel)
 * This perhaps shall be different file, which has to be chosen on compile time
 * depending on OEM needs
 */
class OnDemandChassisProvider {
 public:
  /**
   * Function that retrieves all Chassis available through EntityManager.
   * @param callback a function that shall be called to convert Dbus output into
   * JSON.
   */
  template <typename CallbackFunc>
  void get_chassis_list(CallbackFunc &&callback) {
    const std::array<const char *, 4> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis",
        "xyz.openbmc_project.Inventory.Item.PowerSupply",
        "xyz.openbmc_project.Inventory.Item.System",
    };
    crow::connections::system_bus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code error_code,
            const std::vector<std::string> &resp) {
          // Callback requires vector<string> to retrieve all available chassis
          // list.
          std::vector<std::string> chassis_list;
          if (error_code) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of vector may vary depending on information from Entity Manager,
            // and empty output could not be treated same way as error.
            callback(false, chassis_list);
            return;
          }
          // Iterate over all retrieved ObjectPaths.
          for (const std::string &objpath : resp) {
            std::size_t last_pos = objpath.rfind("/");
            if (last_pos != std::string::npos) {
              // and put it into output vector.
              chassis_list.emplace_back(objpath.substr(last_pos + 1));
            }
          }
          // Finally make a callback with usefull data
          callback(true, chassis_list);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", int32_t(3), interfaces);
  };
};

/**
 * ChassisCollection derived class for delivering Chassis Collection Schema
 */
class ChassisCollection : public Node {
 public:
  template <typename CrowApp>
  ChassisCollection(CrowApp &app) : Node(app, "/redfish/v1/Chassis/") {
    Node::json["@odata.type"] = "#ChassisCollection.ChassisCollection";
    Node::json["@odata.id"] = "/redfish/v1/Chassis";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#ChassisCollection.ChassisCollection";
    Node::json["Name"] = "Chassis Collection";

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
    // Get chassis list, and call the below callback for JSON preparation
    chassis_provider.get_chassis_list(
        [&](const bool &success, const std::vector<std::string> &output) {
          if (success) {
            // ... prepare json array with appropriate @odata.id links
            nlohmann::json chassis_array = nlohmann::json::array();
            for (const std::string &chassis_item : output) {
              chassis_array.push_back(
                  {{"@odata.id", "/redfish/v1/Chassis/" + chassis_item}});
            }
            // Then attach members, count size and return,
            Node::json["Members"] = chassis_array;
            Node::json["Members@odata.count"] = chassis_array.size();
            res.json_value = Node::json;
          } else {
            // ... otherwise, return INTERNALL ERROR
            res.result(boost::beast::http::status::internal_server_error);
          }
          res.end();
        });
  }

  // Chassis Provider object
  // TODO(Pawel) consider move it to singleton
  OnDemandChassisProvider chassis_provider;
};

/**
 * Chassis override class for delivering Chassis Schema
 */
class Chassis : public Node {
 public:
  /*
   * Default Constructor
   */
  template <typename CrowApp>
  Chassis(CrowApp &app)
      : Node(app, "/redfish/v1/Chassis/<str>/", std::string()) {
    Node::json["@odata.type"] = "#Chassis.v1_4_0.Chassis";
    Node::json["@odata.id"] = "/redfish/v1/Chassis";
    Node::json["@odata.context"] = "/redfish/v1/$metadata#Chassis.Chassis";
    Node::json["Name"] = "Chassis Collection";
    Node::json["ChassisType"] = "RackMount";

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
    // Check if there is required param, truly entering this shall be
    // impossible.
    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.end();
      return;
    }

    res.json_value = Node::json;
    const std::string &chassis_id = params[0];
    crow::connections::system_bus->async_method_call(
        [&res, chassis_id(std::string(chassis_id)) ](
            const boost::system::error_code error_code,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>
                &subtree) {
          if (error_code) {
            res.json_value = {};
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
          }
          // Iterate over all retrieved ObjectPaths.
          for (const std::pair<std::string,
                               std::vector<std::pair<std::string,
                                                     std::vector<std::string>>>>
                   &object : subtree) {
            const std::string &path = object.first;
            const std::vector<std::pair<std::string, std::vector<std::string>>>
                &connectionNames = object.second;

            if (!boost::ends_with(path, chassis_id)) {
              continue;
            }
            if (connectionNames.size() < 1) {
              CROW_LOG_ERROR << "Only got " << connectionNames.size()
                             << " connection names";
              continue;
            }

            const std::string connectionName = connectionNames[0].first;
            crow::connections::system_bus->async_method_call(
                [&res, chassis_id(std::string(chassis_id)) ](
                    const boost::system::error_code error_code,
                    const std::vector<std::pair<std::string, VariantType>>
                        &propertiesList) {
                  for (const std::pair<std::string, VariantType> &property :
                       propertiesList) {
                    const std::string *value =
                        mapbox::get_ptr<const std::string>(property.second);
                    if (value != nullptr) {
                      res.json_value[property.first] = *value;
                    }
                  }
                  res.json_value["Name"] = chassis_id;
                  res.json_value["Thermal"] = {
                      {"@odata.id",
                       "/redfish/v1/Chassis/" + chassis_id + "/Thermal"}};
                  res.end();
                },
                connectionName, path, "org.freedesktop.DBus.Properties",
                "GetAll", "xyz.openbmc_project.Inventory.Decorator.Asset");
            // Found the connection we were looking for, return
            return;
          }

          // Couldn't find an object with that name.  return an error
          res.result(boost::beast::http::status::not_found);

          res.end();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char *, 1>{
            "xyz.openbmc_project.Inventory.Decorator.Asset"});
  }

  // Chassis Provider object
  // TODO(Pawel) consider move it to singleton
  OnDemandChassisProvider chassis_provider;
};  // namespace redfish

}  // namespace redfish
