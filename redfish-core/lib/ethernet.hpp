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
using PropertiesMapType = boost::container::flat_map<
    std::string,
    sdbusplus::message::variant<std::string, bool, uint8_t, int16_t, uint16_t,
                                int32_t, uint32_t, int64_t, uint64_t, double>>;

using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string,
        boost::container::flat_map<
            std::string, sdbusplus::message::variant<
                             std::string, bool, uint8_t, int16_t, uint16_t,
                             int32_t, uint32_t, int64_t, uint64_t, double>>>>;

/**
 * Structure for keeping IPv4 data required by Redfish
 * TODO(Pawel) consider change everything to ptr, or to non-ptr values.
 */
struct IPv4AddressData {
  const std::string *address;
  const std::string *domain;
  const std::string *gateway;
  std::string netmask;
  std::string origin;
  bool global;
};

/**
 * Structure for keeping basic single Ethernet Interface information
 * available from DBus
 */
struct EthernetInterfaceData {
  const unsigned int *speed;
  const bool *auto_neg;
  const std::string *hostname;
  const std::string *default_gateway;
  const std::string *mac_address;
  const unsigned int *vlan_id;
};

/**
 * OnDemandEthernetProvider
 * Ethernet provider class that retrieves data directly from dbus, before
 * setting it into JSON output. This does not cache any data.
 *
 * TODO(Pawel)
 * This perhaps shall be different file, which has to be chosen on compile time
 * depending on OEM needs
 */
class OnDemandEthernetProvider {
 private:
  // Consts that may have influence on EthernetProvider performance/memory usage
  const size_t MAX_IPV4_ADDRESSES_PER_INTERFACE = 10;

  // Helper function that allows to extract GetAllPropertiesType from
  // GetManagedObjectsType, based on object path, and interface name
  const PropertiesMapType *extractInterfaceProperties(
      const sdbusplus::message::object_path &objpath,
      const std::string &interface, const GetManagedObjectsType &dbus_data) {
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
  // into sdbusplus::message::object_path type
  inline const PropertiesMapType *extractInterfaceProperties(
      const std::string &objpath, const std::string &interface,
      const GetManagedObjectsType &dbus_data) {
    const auto &dbus_obj = sdbusplus::message::object_path{objpath};
    return extractInterfaceProperties(dbus_obj, interface, dbus_data);
  }

  // Helper function that allows to get pointer to the property from
  // GetAllPropertiesType native, or extracted by GetAllPropertiesType
  template <typename T>
  inline T const *const extractProperty(const PropertiesMapType &properties,
                                        const std::string &name) {
    const auto &property = properties.find(name);
    if (property != properties.end()) {
      return mapbox::get_ptr<const T>(property->second);
    }
    return nullptr;
  }
  // TODO(Pawel) Consider to move the above functions to dbus
  // generic_interfaces.hpp

  // Helper function that extracts data from several dbus objects and several
  // interfaces required by single ethernet interface instance
  void extractEthernetInterfaceData(const std::string &ethiface_id,
                                    const GetManagedObjectsType &dbus_data,
                                    EthernetInterfaceData &eth_data) {
    // Extract data that contains MAC Address
    const PropertiesMapType *mac_properties = extractInterfaceProperties(
        "/xyz/openbmc_project/network/" + ethiface_id,
        "xyz.openbmc_project.Network.MACAddress", dbus_data);

    if (mac_properties != nullptr) {
      eth_data.mac_address =
          extractProperty<std::string>(*mac_properties, "MACAddress");
    }

    const PropertiesMapType *vlan_properties = extractInterfaceProperties(
        "/xyz/openbmc_project/network/" + ethiface_id,
        "xyz.openbmc_project.Network.VLAN", dbus_data);

    if (vlan_properties != nullptr) {
      eth_data.vlan_id = extractProperty<unsigned int>(*vlan_properties, "Id");
    }

    // Extract data that contains link information (auto negotiation and speed)
    const PropertiesMapType *eth_properties = extractInterfaceProperties(
        "/xyz/openbmc_project/network/" + ethiface_id,
        "xyz.openbmc_project.Network.EthernetInterface", dbus_data);

    if (eth_properties != nullptr) {
      eth_data.auto_neg = extractProperty<bool>(*eth_properties, "AutoNeg");
      eth_data.speed = extractProperty<unsigned int>(*eth_properties, "Speed");
    }

    // Extract data that contains network config (HostName and DefaultGW)
    const PropertiesMapType *config_properties = extractInterfaceProperties(
        "/xyz/openbmc_project/network/config",
        "xyz.openbmc_project.Network.SystemConfiguration", dbus_data);

    if (config_properties != nullptr) {
      eth_data.hostname =
          extractProperty<std::string>(*config_properties, "HostName");
      eth_data.default_gateway =
          extractProperty<std::string>(*config_properties, "DefaultGateway");
    }
  }

  // Helper function that changes bits netmask notation (i.e. /24)
  // into full dot notation
  inline std::string getNetmask(unsigned int bits) {
    uint32_t value = 0xffffffff << (32 - bits);
    std::string netmask = std::to_string((value >> 24) & 0xff) + "." +
                          std::to_string((value >> 16) & 0xff) + "." +
                          std::to_string((value >> 8) & 0xff) + "." +
                          std::to_string(value & 0xff);
    return netmask;
  }

  // Helper function that extracts data for single ethernet ipv4 address
  void extractIPv4Data(const std::string &ethiface_id,
                       const GetManagedObjectsType &dbus_data,
                       std::vector<IPv4AddressData> &ipv4_config) {
    // Since there might be several IPv4 configurations aligned with
    // single ethernet interface, loop over all of them
    for (auto &objpath : dbus_data) {
      // Check if proper patter for object path appears
      if (boost::starts_with(
              static_cast<const std::string&>(objpath.first),
              "/xyz/openbmc_project/network/" + ethiface_id + "/ipv4/")) {
        // and get approrpiate interface
        const auto &interface =
            objpath.second.find("xyz.openbmc_project.Network.IP");
        if (interface != objpath.second.end()) {
          // Make a properties 'shortcut', to make everything more readable
          const PropertiesMapType &properties = interface->second;
          // Instance IPv4AddressData structure, and set as appropriate
          IPv4AddressData ipv4_address;
          // IPv4 address
          ipv4_address.address =
              extractProperty<std::string>(properties, "Address");
          // IPv4 gateway
          ipv4_address.gateway =
              extractProperty<std::string>(properties, "Gateway");

          // Origin is kind of DBus object so fetch pointer...
          const std::string *origin =
              extractProperty<std::string>(properties, "Origin");
          if (origin != nullptr) {
            // ... and get everything after last dot
            int last = origin->rfind(".");
            if (last != std::string::npos) {
              ipv4_address.origin = origin->substr(last + 1);
            }
          }

          // Netmask is presented as PrefixLength
          const auto *mask =
              extractProperty<uint8_t>(properties, "PrefixLength");
          if (mask != nullptr) {
            // convert it to the string
            ipv4_address.netmask = getNetmask(*mask);
          }

          // Attach IPv4 only if address is present
          if (ipv4_address.address != nullptr) {
            // Check if given address is local, or global
            if (boost::starts_with(*ipv4_address.address, "169.254")) {
              ipv4_address.global = false;
            } else {
              ipv4_address.global = true;
            }
            ipv4_config.emplace_back(std::move(ipv4_address));
          }
        }
      }
    }
  }

 public:
  /**
   * Function that retrieves all properties for given Ethernet Interface Object
   * from EntityManager Network Manager
   * @param ethiface_id a eth interface id to query on DBus
   * @param callback a function that shall be called to convert Dbus output into
   * JSON
   */
  template <typename CallbackFunc>
  void getEthernetIfaceData(const std::string &ethiface_id,
                            CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [
          this, ethiface_id{std::move(ethiface_id)},
          callback{std::move(callback)}
        ](const boost::system::error_code error_code,
          GetManagedObjectsType &resp) {

          EthernetInterfaceData eth_data{};
          std::vector<IPv4AddressData> ipv4_data;
          ipv4_data.reserve(MAX_IPV4_ADDRESSES_PER_INTERFACE);

          if (error_code) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of vector may vary depending on information from Network Manager,
            // and empty output could not be treated same way as error.
            callback(false, eth_data, ipv4_data);
            return;
          }

          extractEthernetInterfaceData(ethiface_id, resp, eth_data);
          extractIPv4Data(ethiface_id, resp, ipv4_data);

          // Fix global GW
          for (IPv4AddressData &ipv4 : ipv4_data) {
            if ((ipv4.global) &&
                ((ipv4.gateway == nullptr) || (*ipv4.gateway == "0.0.0.0"))) {
              ipv4.gateway = eth_data.default_gateway;
            }
          }

          // Finally make a callback with useful data
          callback(true, eth_data, ipv4_data);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
  };

  /**
   * Function that retrieves all Ethernet Interfaces available through Network
   * Manager
   * @param callback a function that shall be called to convert Dbus output into
   * JSON.
   */
  template <typename CallbackFunc>
  void getEthernetIfaceList(CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [ this, callback{std::move(callback)} ](
            const boost::system::error_code error_code,
            GetManagedObjectsType &resp) {
          // Callback requires vector<string> to retrieve all available ethernet
          // interfaces
          std::vector<std::string> iface_list;
          iface_list.reserve(resp.size());
          if (error_code) {
            // Something wrong on DBus, the error_code is not important at this
            // moment, just return success=false, and empty output. Since size
            // of vector may vary depending on information from Network Manager,
            // and empty output could not be treated same way as error.
            callback(false, iface_list);
            return;
          }

          // Iterate over all retrieved ObjectPaths.
          for (auto &objpath : resp) {
            // And all interfaces available for certain ObjectPath.
            for (auto &interface : objpath.second) {
              // If interface is xyz.openbmc_project.Network.EthernetInterface,
              // this is what we're looking for.
              if (interface.first ==
                  "xyz.openbmc_project.Network.EthernetInterface") {
                // Cut out everyting until last "/", ...
                const std::string& iface_id =
                    static_cast<const std::string&>(objpath.first);
                std::size_t last_pos = iface_id.rfind("/");
                if (last_pos != std::string::npos) {
                  // and put it into output vector.
                  iface_list.emplace_back(iface_id.substr(last_pos + 1));
                }
              }
            }
          }
          // Finally make a callback with useful data
          callback(true, iface_list);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
  };
};

/**
 * EthernetCollection derived class for delivering Ethernet Collection Schema
 */
class EthernetCollection : public Node {
 public:
  template <typename CrowApp>
  // TODO(Pawel) Remove line from below, where we assume that there is only one
  // manager called openbmc This shall be generic, but requires to update
  // GetSubroutes method
  EthernetCollection(CrowApp &app)
      : Node(app, "/redfish/v1/Managers/openbmc/EthernetInterfaces/") {
    Node::json["@odata.type"] =
        "#EthernetInterfaceCollection.EthernetInterfaceCollection";
    Node::json["@odata.context"] =
        "/redfish/v1/"
        "$metadata#EthernetInterfaceCollection.EthernetInterfaceCollection";
    Node::json["@odata.id"] = "/redfish/v1/Managers/openbmc/EthernetInterfaces";
    Node::json["Name"] = "Ethernet Network Interface Collection";
    Node::json["Description"] =
        "Collection of EthernetInterfaces for this Manager";

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
    // TODO(Pawel) this shall be parametrized call to get EthernetInterfaces for
    // any Manager, not only hardcoded 'openbmc'.
    std::string manager_id = "openbmc";

    // Get eth interface list, and call the below callback for JSON preparation
    ethernet_provider.getEthernetIfaceList(
        [&, manager_id{std::move(manager_id)} ](
            const bool &success, const std::vector<std::string> &iface_list) {
          if (success) {
            nlohmann::json iface_array = nlohmann::json::array();
            for (const std::string &iface_item : iface_list) {
              iface_array.push_back(
                  {{"@odata.id", "/redfish/v1/Managers/" + manager_id +
                                     "/EthernetInterfaces/" + iface_item}});
            }
            Node::json["Members"] = iface_array;
            Node::json["Members@odata.count"] = iface_array.size();
            Node::json["@odata.id"] =
                "/redfish/v1/Managers/" + manager_id + "/EthernetInterfaces";
            res.json_value = Node::json;
          } else {
            // No success, best what we can do is return INTERNALL ERROR
            res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
          }
          res.end();
        });
  }

  // Ethernet Provider object
  // TODO(Pawel) consider move it to singleton
  OnDemandEthernetProvider ethernet_provider;
};

/**
 * EthernetInterface derived class for delivering Ethernet Schema
 */
class EthernetInterface : public Node {
 public:
  /*
   * Default Constructor
   */
  template <typename CrowApp>
  // TODO(Pawel) Remove line from below, where we assume that there is only one
  // manager called openbmc This shall be generic, but requires to update
  // GetSubroutes method
  EthernetInterface(CrowApp &app)
      : Node(app, "/redfish/v1/Managers/openbmc/EthernetInterfaces/<str>/",
             std::string()) {
    Node::json["@odata.type"] = "#EthernetInterface.v1_2_0.EthernetInterface";
    Node::json["@odata.context"] =
        "/redfish/v1/$metadata#EthernetInterface.EthernetInterface";
    Node::json["Name"] = "Manager Ethernet Interface";
    Node::json["Description"] = "Management Network Interface";

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
    // TODO(Pawel) this shall be parametrized call (two params) to get
    // EthernetInterfaces for any Manager, not only hardcoded 'openbmc'.
    // Check if there is required param, truly entering this shall be
    // impossible.
    if (params.size() != 1) {
      res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
      res.end();
      return;
    }

    const std::string &iface_id = params[0];

    // Get single eth interface data, and call the below callback for JSON
    // preparation
    ethernet_provider.getEthernetIfaceData(
        iface_id, [&, iface_id](const bool &success,
                                const EthernetInterfaceData &eth_data,
                                const std::vector<IPv4AddressData> &ipv4_data) {
          if (success) {
            // Copy JSON object to avoid race condition
            nlohmann::json json_response(Node::json);

            // Fill out obvious data...
            json_response["Id"] = iface_id;
            json_response["@odata.id"] =
                "/redfish/v1/Managers/openbmc/EthernetInterfaces/" + iface_id;

            // ... then the one from DBus, regarding eth iface...
            if (eth_data.speed != nullptr)
              json_response["SpeedMbps"] = *eth_data.speed;

            if (eth_data.mac_address != nullptr)
              json_response["MACAddress"] = *eth_data.mac_address;

            if (eth_data.hostname != nullptr)
              json_response["HostName"] = *eth_data.hostname;

            if (eth_data.vlan_id != nullptr) {
              json_response["VLAN"]["VLANEnable"] = true;
              json_response["VLAN"]["VLANId"] = *eth_data.vlan_id;
            }

            // ... at last, check if there are IPv4 data and prepare appropriate
            // collection
            if (ipv4_data.size() > 0) {
              nlohmann::json ipv4_array = nlohmann::json::array();
              for (auto &ipv4_config : ipv4_data) {
                nlohmann::json json_ipv4;
                if (ipv4_config.address != nullptr) {
                  json_ipv4["Address"] = *ipv4_config.address;
                  if (ipv4_config.gateway != nullptr)
                    json_ipv4["Gateway"] = *ipv4_config.gateway;

                  json_ipv4["AddressOrigin"] = ipv4_config.origin;
                  json_ipv4["SubnetMask"] = ipv4_config.netmask;

                  ipv4_array.push_back(json_ipv4);
                }
              }
              json_response["IPv4Addresses"] = ipv4_array;
            }
            res.json_value = std::move(json_response);
          } else {
            // ... otherwise return error
            // TODO(Pawel)consider distinguish between non existing object, and
            // other errors
            res.code = static_cast<int>(HttpRespCode::NOT_FOUND);
          }
          res.end();
        });
  }

  // Ethernet Provider object
  // TODO(Pawel) consider move it to singleton
  OnDemandEthernetProvider ethernet_provider;
};

}  // namespace redfish
