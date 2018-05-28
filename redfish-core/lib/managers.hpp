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

/**
 * DBus types primitives for several generic DBus interfaces
 * TODO consider move this to separate file into boost::dbus
 */
using PropertiesMapType =
    boost::container::flat_map<std::string, dbus::dbus_variant>;

using GetManagedObjectsType = boost::container::flat_map<
    dbus::object_path,
    boost::container::flat_map<std::string, PropertiesMapType>>;

using GetAllPropertiesType = PropertiesMapType;

/**
 * Structure for keeping BMC Updater Software Interface information
 * available from DBus
 */
struct SoftwareVersion {
  const std::string *version;
  const std::string *purpose;
};

/**
 * OnDemandSoftwareProvider
 * Software provider class that retrieves data directly from dbus, before seting
 * it into JSON output. This does not cache any data.
 *
 * TODO
 * This perhaps shall be different file, which has to be chosen on compile time
 * depending on OEM needs
 */
class OnDemandSoftwareProvider {
 private:
  // Helper function that allows to extract GetAllPropertiesType from
  // GetManagedObjectsType, based on object path, and interface name
  const PropertiesMapType *extractInterfaceProperties(
      const dbus::object_path &objpath, const std::string &interface,
      const GetManagedObjectsType &dbus_data) {
    const auto &dbus_obj = dbus_data.find(objpath);
    if (dbus_obj != dbus_data.end()) {
      const auto &iface = dbus_obj->second.find(interface);
      if (iface != dbus_obj->second.end()) {
        return &iface->second;
      }
    }
    return nullptr;
  }

  // Helper Wrapper that does inline object_path conversion from string
  // into dbus::object_path type
  inline const PropertiesMapType *extractInterfaceProperties(
      const std::string &objpath, const std::string &interface,
      const GetManagedObjectsType &dbus_data) {
    const auto &dbus_obj = dbus::object_path{objpath};
    return extractInterfaceProperties(dbus_obj, interface, dbus_data);
  }

  // Helper function that allows to get pointer to the property from
  // GetAllPropertiesType native, or extracted by GetAllPropertiesType
  template <typename T>
  inline const T *extractProperty(const PropertiesMapType &properties,
                                  const std::string &name) {
    const auto &property = properties.find(name);
    if (property != properties.end()) {
      return boost::get<T>(&property->second);
    }
    return nullptr;
  }
  // TODO Consider to move the above functions to dbus
  // generic_interfaces.hpp

  // Helper function that extracts data from dbus object and
  // interface required by single software interface instance
  void extractSoftwareVersionIfaceData(const std::string &bmcIface_id,
                                    const GetManagedObjectsType &dbus_data,
                                    SoftwareVersion &soft_ver) {
    // Extract data that contains software version information
    const PropertiesMapType *soft_ver_properties = extractInterfaceProperties(
        "/xyz/openbmc_project/software/" + bmcIface_id,
        "xyz.openbmc_project.Software.Version", dbus_data);

    if (soft_ver_properties != nullptr) {
      soft_ver.version =
          extractProperty<std::string>(*soft_ver_properties, "Version");
      soft_ver.purpose =
          extractProperty<std::string>(*soft_ver_properties, "Purpose");
    }
  }

 public:
  /**
   * Function that retrieves Software Version property for given Managed Object
   * from Software Manager
   * @param callback a function that shall be called to convert Dbus output into
   * JSON
   */
  template <typename CallbackFunc>
  void getSoftwareIfaceData(CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [
          this, callback{std::move(callback)}
        ](const boost::system::error_code error_code,
          const GetManagedObjectsType &resp) {

          SoftwareVersion soft_ver{};

          if (error_code) {
            // TODO Something wrong on DBus, the error_code is not important at
            // this moment, just return success=false, and empty output.
            callback(false, soft_ver);
            return;
          }

          // Iterate over all retrieved ObjectPaths.
          for (auto &objpath : resp) {
            // And all interfaces available for certain ObjectPath.
            for (auto &interface : objpath.second) {
              // If interface is xyz.openbmc_project.Software.Version,
              // this is what we're looking for.
              if (interface.first ==
                  "xyz.openbmc_project.Software.Version") {
                // Cut out everyting until last "/", ...
                const std::string &iface_id = objpath.first.value;
                std::size_t last_pos = iface_id.rfind("/");
                if (last_pos != std::string::npos) {
                  extractSoftwareVersionIfaceData(iface_id.substr(last_pos + 1),
                                                  resp, soft_ver);
                }
              }
            }
          }

          // Finally make a callback with usefull data
          callback(true, soft_ver);
        },
        {"xyz.openbmc_project.Software.BMC.Updater",
         "/xyz/openbmc_project/software",
         "org.freedesktop.DBus.ObjectManager", "GetManagedObjects"});
  };
};

/**
 * ManagerActionsReset class supports handle POST method for Reset action.
 * The class retrieves and sends data directly to dbus.
 */
class ManagerActionsReset : public Node {
 public:
  ManagerActionsReset(CrowApp& app)
      : Node(app, "/redfish/v1/Managers/bmc/Actions/Manager.Reset/") {

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureManager"}}}};
  }

 private:
  /**
   * Function handles GET method request.
   * ManagerActionReset supports for POST method,
   * it is not required to retrieve more information in GET.
   */
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    res.json_value = Node::json;
    res.end();
  }

  /**
   * Function handles POST method request.
   * Analyzes POST body message before sends Reset request data to dbus.
   * OpenBMC allows for ResetType is GracefulRestart only.
   */
  void doPost(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    // Parse request data to JSON object
    auto request_json_obj = nlohmann::json::parse(req.body, nullptr, false);
    if (request_json_obj.is_discarded()) {
      res.code = static_cast<int>(HttpRespCode::BAD_REQUEST);
      CROW_LOG_ERROR << "Incorrect body message for POST.";
      res.end();
      return;
    }

    // Within the Reset action, the POST request should be only 1 JSON element.
    if (request_json_obj.size() == 1) {
      nlohmann::json::iterator it = request_json_obj.begin();
      // Extract data
      std::string data = it.key();
      if (data  == "ResetType") {
        // Extract property value before invoke the corresponding method.
        const std::string &property_value = it->get<const std::string>();
        // OpenBMC only allows for GracefulRestart property.
        if (property_value == "GracefulRestart") {
          doBMCGracefulRestart(res, req, params);
        } else {
          res.code = static_cast<int>(HttpRespCode::BAD_REQUEST);
          CROW_LOG_ERROR << "Request incorrect property: " << property_value;
          res.end();
          return;
        }
      } else {
        res.code = static_cast<int>(HttpRespCode::BAD_REQUEST);
        CROW_LOG_ERROR << "Incorrect request data: Only ResetType is accepted.";
        res.end();
        return;
      }
    } else {
      res.code = static_cast<int>(HttpRespCode::BAD_REQUEST);
      CROW_LOG_ERROR << "Only one JSON element is accepted in Reset action.";
      res.end();
      return;
    }
  }

  /**
   * Function transceives data with dbus directly.
   * All BMC state properties will be retrieved before sending reset request.
   */
  void doBMCGracefulRestart(crow::response& res,
                            const crow::request& req,
                            const std::vector<std::string>& params) {
    const std::string &process_name = "xyz.openbmc_project.State.BMC";
    const std::string &object_path = "/xyz/openbmc_project/state/bmc0";
    const std::string &interface_name = "xyz.openbmc_project.State.BMC";
    const std::string &property_value =
                            "xyz.openbmc_project.State.BMC.Transition.Reboot";
    const std::string &dest_property = "RequestedBMCTransition";

    // Create the D-Bus variant for D-Bus call.
    dbus::dbus_variant dbus_property_value(property_value);
    crow::connections::system_bus->async_method_call(
      [&,
        object_path, process_name,
        interface_name, dest_property,
        dbus_property_value{std::move(dbus_property_value)}
      ](const boost::system::error_code ec,
        const PropertiesMapType &properties) {
        if (ec) {
          CROW_LOG_ERROR << "[GetAll] Bad D-Bus request error: " << ec;
          res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
          res.end();
          return;
        } else {
          auto it = properties.find(dest_property);
          if (it != properties.end()) {
            crow::connections::system_bus->async_method_call(
                [&](const boost::system::error_code ec) {
                  // Use "Set" method to set the property value.
                  if (ec) {
                    CROW_LOG_ERROR << "[Set] Bad D-Bus request error: " << ec;
                    res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
                    res.end();
                    return;
                  }
                },
                {process_name, object_path,
                "org.freedesktop.DBus.Properties", "Set"},
                interface_name, dest_property, dbus_property_value);
          } else {
            CROW_LOG_ERROR << "Not found the requested property: "
                            << dest_property;
            res.code = static_cast<int>(HttpRespCode::NOT_FOUND);
            res.end();
            return;
          }
        }
      },
      {process_name, object_path,
      "org.freedesktop.DBus.Properties", "GetAll"},
      interface_name);
    res.code = static_cast<int>(HttpRespCode::NO_CONTENT);
    CROW_LOG_DEBUG << "Response with no content";
    res.json_value = Node::json;
    res.end();
  }
};

class Manager : public Node {
 public:
  Manager(CrowApp& app) : Node(app, "/redfish/v1/Managers/bmc/"),
                          memberActionsReset(app) {
    Node::json["@odata.id"] = "/redfish/v1/Managers/bmc";
    Node::json["@odata.type"] = "#Manager.v1_3_0.Manager";
    Node::json["@odata.context"] = "/redfish/v1/$metadata#Manager.Manager";
    Node::json["Id"] = "bmc";
    Node::json["Name"] = "OpenBmc Manager";
    Node::json["Description"] = "Baseboard Management Controller";
    Node::json["PowerState"] = "On";
    Node::json["ManagerType"] = "BMC";
    Node::json["UUID"] =
        app.template get_middleware<crow::PersistentData::Middleware>()
            .system_uuid;
    Node::json["Oem"] = {
        {"AmpereComputing", {
            {"@odata.type", "https://amperecomputing.com"},
            {"ProductionLocation", {
                {"FacilityName", "Ampere Computing Production Facility"},
                {"Country", "United States"}
            }}
        }}
    };  // TODO, read the OEM info from FRU or refactor to make it common,
        // this is hardcoded for now
    Node::json["Model"] = "OpenBmc";               // TODO(ed), get model
    Node::json["NetworkProtocol"] = nlohmann::json(
        {{"@odata.id",
          "/redfish/v1/Managers/bmc/NetworkProtocol"}});
    Node::json["EthernetInterfaces"] = nlohmann::json(
        {{"@odata.id",
          "/redfish/v1/Managers/bmc/EthernetInterfaces"}});  // TODO(Pawel),
                                                             // remove this
                                                             // when
                                                             // subroutes
                                                             // will work
                                                             // correctly
    Node::json["Links"]["ManagerForServers"] = {
                            {{"@odata.id", "/redfish/v1/Systems/1"}}};
    Node::json["Links"]["ManagerForChassis"] = {
                            {{"@odata.id", "/redfish/v1/Chassis/1"}}};
    Node::json["Links"]["ManagerInChassis"] = {
                            {"@odata.id", "/redfish/v1/Chassis/1"}};

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureManager"}}}};
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    software_provider.getSoftwareIfaceData(
        [&](const bool &success, const SoftwareVersion &soft_ver) {
          if (success) {
            Node::json["FirmwareVersion"] = *soft_ver.version;
          } else {
            // No success, notify error message
            CROW_LOG_ERROR << "Error while getting Software Version";
            Node::json["FirmwareVersion"] = "INTERNAL ERROR";
          }
        });

    // Update Actions object.
    nlohmann::json reset_type = nlohmann::json::array();
    // OpenBMC supports only graceful shutdown followed by a restart.
    reset_type.push_back("GracefulRestart");
    nlohmann::json manager_reset;
    manager_reset["target"] =
                          "/redfish/v1/Managers/bmc/Actions/Manager.Reset";
    manager_reset["ResetType@Redfish.AllowableValues"] = reset_type;
    Node::json["Actions"]["#Manager.Reset"] = manager_reset;

    Node::json["DateTime"] = getDateTime();
    res.json_value = Node::json;
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

  // Software Provider object
  // TODO consider move it to singleton
  OnDemandSoftwareProvider software_provider;
  // Actions Reset object as a member of Manager resource.
  // Handle reset action from POST request.
  ManagerActionsReset memberActionsReset;

};

class ManagerCollection : public Node {
 public:
  ManagerCollection(CrowApp& app)
      : Node(app, "/redfish/v1/Managers/"), memberManager(app) {
    Node::json["@odata.id"] = "/redfish/v1/Managers";
    Node::json["@odata.type"] = "#ManagerCollection.ManagerCollection";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#ManagerCollection.ManagerCollection";
    Node::json["Name"] = "Manager Collection";
    Node::json["Members@odata.count"] = 1;
    Node::json["Members"] = {{{"@odata.id", "/redfish/v1/Managers/bmc"}}};

    entityPrivileges = {{crow::HTTPMethod::GET, {{"Login"}}},
                        {crow::HTTPMethod::HEAD, {{"Login"}}},
                        {crow::HTTPMethod::PATCH, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::PUT, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::DELETE, {{"ConfigureManager"}}},
                        {crow::HTTPMethod::POST, {{"ConfigureManager"}}}};
  }

 private:
  void doGet(crow::response& res, const crow::request& req,
             const std::vector<std::string>& params) override {
    res.json_value = Node::json;
    res.end();
  }

  Manager memberManager;
};

}  // namespace redfish
