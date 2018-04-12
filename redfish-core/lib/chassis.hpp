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
 * Chassis provider class that retrieves data directly from dbus, before setting
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
   * Function that retrieves all properties for given Chassis Object from
   * EntityManager
   * @param res_name a chassis resource name to query on DBus
   * @param callback a function that shall be called to convert Dbus output into
   * JSON
   */
  template <typename CallbackFunc>
  void get_chassis_data(const std::string &res_name, CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code error_code,
            const PropertiesType &properties) {
          // Callback requires flat_map<string, string> so prepare one.
          boost::container::flat_map<std::string, std::string> output;
          if (error_code) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of map may vary depending on Chassis type, an empty output could
            // not be treated same way as error.
            callback(false, output);
            return;
          }
          for (const std::pair<const char *, const char *> &p :
               std::array<std::pair<const char *, const char *>, 5>{
                   {{"name", "Name"},
                    {"manufacturer", "Manufacturer"},
                    {"model", "Model"},
                    {"part_number", "PartNumber"},
                    {"serial_number", "SerialNumber"}}}) {
            PropertiesType::const_iterator it = properties.find(p.first);
            if (it != properties.end()) {
              const std::string *s =
                  mapbox::get_ptr<const std::string>(it->second);
              if (s != nullptr) {
                output[p.second] = *s;
              }
            }
          }
          // Callback with success, and hopefully data.
          callback(true, output);
        },
        "xyz.openbmc_project.EntityManager",
        "/xyz/openbmc_project/Inventory/Item/Chassis/" + res_name,
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Configuration.Chassis");
  }

  /**
   * Function that retrieves all Chassis available through EntityManager.
   * @param callback a function that shall be called to convert Dbus output into
   * JSON.
   */
  template <typename CallbackFunc>
  void get_chassis_list(CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code error_code,
            const ManagedObjectsType &resp) {
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
          for (auto &objpath : resp) {
            // And all interfaces available for certain ObjectPath.
            for (auto &interface : objpath.second) {
              // If interface is xyz.openbmc_project.Configuration.Chassis, this
              // is Chassis.
              if (interface.first ==
                  "xyz.openbmc_project.Configuration.Chassis") {
                // Cut out everything until last "/", ...
                const std::string &chassis_id = objpath.first.str;
                std::size_t last_pos = chassis_id.rfind("/");
                if (last_pos != std::string::npos) {
                  // and put it into output vector.
                  chassis_list.emplace_back(chassis_id.substr(last_pos + 1));
                }
              }
            }
          }
          // Finally make a callback with useful data
          callback(true, chassis_list);
        },
        "xyz.openbmc_project.EntityManager",
        "/xyz/openbmc_project/Inventory/Item/Chassis",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureComponents"}}}};
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
            res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
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

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureComponents"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureComponents"}}}};
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
      res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
      res.end();
      return;
    }
    const std::string &chassis_id = params[0];
    // Get certain Chassis Data, and call the below callback for JSON
    // preparation lambda requires everything as reference, and chassis_id,
    // which is local by copy
    chassis_provider.get_chassis_data(
        chassis_id,
        [&, chassis_id](const bool &success,
                        const boost::container::flat_map<std::string,
                                                         std::string> &output) {
          // Create JSON copy based on Node::json, this is to avoid possible
          // race condition
          nlohmann::json json_response(Node::json);
          json_response["Thermal"] = {
              {"@odata.id", "/redfish/v1/Chassis/" + chassis_id + "/Thermal"}};
          // If success...
          if (success) {
            // prepare all the schema required fields.
            json_response["@odata.id"] = "/redfish/v1/Chassis/" + chassis_id;
            // also the one from dbus
            for (const std::pair<std::string, std::string> &chassis_item :
                 output) {
              json_response[chassis_item.first] = chassis_item.second;
            }

            json_response["Id"] = chassis_id;
            // prepare respond, and send
            res.json_value = json_response;
          } else {
            // ... otherwise return error
            // TODO(Pawel)consider distinguish between non existing object, and
            // other errors
            res.code = static_cast<int>(HttpRespCode::NOT_FOUND);
          }
          res.end();
        });
  }

  // Chassis Provider object
  // TODO(Pawel) consider move it to singleton
  OnDemandChassisProvider chassis_provider;
};

}  // namespace redfish
