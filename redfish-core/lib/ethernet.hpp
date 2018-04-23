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

#include <dbus_singleton.hpp>
#include <error_messages.hpp>
#include <node.hpp>
#include <utils/json_utils.hpp>
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
  std::string id;
  const std::string *address;
  const std::string *domain;
  const std::string *gateway;
  std::string netmask;
  std::string origin;
  bool global;
  /**
   * @brief Operator< to enable sorting
   *
   * @param[in] obj   Object to compare with
   *
   * @return This object id < supplied object id
   */
  bool operator<(const IPv4AddressData &obj) const { return (id < obj.id); }
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
    const std::string pathStart =
        "/xyz/openbmc_project/network/" + ethiface_id + "/ipv4/";

    // Since there might be several IPv4 configurations aligned with
    // single ethernet interface, loop over all of them
    for (auto &objpath : dbus_data) {
      // Check if proper patter for object path appears
      if (boost::starts_with(
              static_cast<const std::string &>(objpath.first),
              "/xyz/openbmc_project/network/" + ethiface_id + "/ipv4/")) {
        // and get approrpiate interface
        const auto &interface =
            objpath.second.find("xyz.openbmc_project.Network.IP");
        if (interface != objpath.second.end()) {
          // Make a properties 'shortcut', to make everything more readable
          const PropertiesMapType &properties = interface->second;
          // Instance IPv4AddressData structure, and set as appropriate
          IPv4AddressData ipv4_address;

          ipv4_address.id = static_cast<const std::string &>(objpath.first)
                                .substr(pathStart.size());

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
            ipv4_address.origin =
                translateAddressOriginBetweenDBusAndRedfish(origin, true, true);
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

    /**
     * We have to sort this vector and ensure that order of IPv4 addresses
     * is consistent between GETs to allow modification and deletion in PATCHes
     */
    std::sort(ipv4_config.begin(), ipv4_config.end());
  }

  static const constexpr int ipV4AddressSectionsCount = 4;

 public:
  /**
   * @brief Creates VLAN for given interface with given Id through D-Bus
   *
   * @param[in] ifaceId       Id of interface for which VLAN will be created
   * @param[in] inputVlanId   ID of the new VLAN
   * @param[in] callback      Function that will be called after the operation
   *
   * @return None.
   */
  template <typename CallbackFunc>
  void createVlan(const std::string &ifaceId, const uint64_t &inputVlanId,
                  CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        callback, "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "xyz.openbmc_project.Network.VLAN.Create", "VLAN", ifaceId,
        static_cast<uint32_t>(inputVlanId));
  };

  /**
   * @brief Sets given Id on the given VLAN interface through D-Bus
   *
   * @param[in] ifaceId       Id of VLAN interface that should be modified
   * @param[in] inputVlanId   New ID of the VLAN
   * @param[in] callback      Function that will be called after the operation
   *
   * @return None.
   */
  template <typename CallbackFunc>
  void changeVlanId(const std::string &ifaceId, const uint32_t &inputVlanId,
                    CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        callback, "xyz.openbmc_project.Network",
        std::string("/xyz/openbmc_project/network/") + ifaceId,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.VLAN", "Id",
        sdbusplus::message::variant<uint32_t>(inputVlanId));
  };

  /**
   * @brief Helper function that verifies IP address to check if it is in
   *        proper format. If bits pointer is provided, also calculates active
   *        bit count for Subnet Mask.
   *
   * @param[in]  ip     IP that will be verified
   * @param[out] bits   Calculated mask in bits notation
   *
   * @return true in case of success, false otherwise
   */
  bool ipv4VerifyIpAndGetBitcount(const std::string &ip,
                                  uint8_t *bits = nullptr) {
    std::vector<std::string> bytesInMask;

    boost::split(bytesInMask, ip, boost::is_any_of("."));

    if (bytesInMask.size() != ipV4AddressSectionsCount) {
      return false;
    }

    if (bits != nullptr) {
      *bits = 0;
    }

    char *endPtr;
    long previousValue = 255;
    bool firstZeroInByteHit;
    for (uint8_t byteIdx = 0; byteIdx < ipV4AddressSectionsCount; byteIdx++) {
      // Use strtol instead of stroi to avoid exceptions
      long value = std::strtol(bytesInMask[byteIdx].c_str(), &endPtr, 10);

      // endPtr should point to the end of the string, otherwise given string
      // is not 100% number
      if (*endPtr != '\0') {
        return false;
      }

      // Value should be contained in byte
      if (value < 0 || value > 255) {
        return false;
      }

      if (bits != nullptr) {
        // Mask has to be continuous between bytes
        if (previousValue != 255 && value != 0) {
          return false;
        }

        // Mask has to be continuous inside bytes
        firstZeroInByteHit = false;

        // Count bits
        for (int bitIdx = 7; bitIdx >= 0; bitIdx--) {
          if (value & (1 << bitIdx)) {
            if (firstZeroInByteHit) {
              // Continuity not preserved
              return false;
            } else {
              (*bits)++;
            }
          } else {
            firstZeroInByteHit = true;
          }
        }
      }

      previousValue = value;
    }

    return true;
  }

  /**
   * @brief Changes IPv4 address type property (Address, Gateway)
   *
   * @param[in] ifaceId     Id of interface whose IP should be modified
   * @param[in] ipIdx       Index of IP in input array that should be modified
   * @param[in] ipHash      DBus Hash id of modified IP
   * @param[in] name        Name of field in JSON representation
   * @param[in] newValue    New value that should be written
   * @param[io] asyncResp   Response object that will be returned to client
   *
   * @return true if give IP is valid and has been sent do D-Bus, false
   * otherwise
   */
  void changeIPv4AddressProperty(const std::string &ifaceId, int ipIdx,
                                 const std::string &ipHash,
                                 const std::string &name,
                                 const std::string &newValue,
                                 const std::shared_ptr<AsyncResp> &asyncResp) {
    auto callback = [
      asyncResp, ipIdx{std::move(ipIdx)}, name{std::move(name)},
      newValue{std::move(newValue)}
    ](const boost::system::error_code ec) {
      if (ec) {
        messages::addMessageToJson(
            asyncResp->res.json_value, messages::internalError(),
            "/IPv4Addresses/" + std::to_string(ipIdx) + "/" + name);
      } else {
        asyncResp->res.json_value["IPv4Addresses"][ipIdx][name] = newValue;
      }
    };

    crow::connections::system_bus->async_method_call(
        std::move(callback), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", name,
        sdbusplus::message::variant<std::string>(newValue));
  };

  /**
   * @brief Changes IPv4 address origin property
   *
   * @param[in] ifaceId       Id of interface whose IP should be modified
   * @param[in] ipIdx         Index of IP in input array that should be modified
   * @param[in] ipHash        DBus Hash id of modified IP
   * @param[in] newValue      New value in Redfish format
   * @param[in] newValueDbus  New value in D-Bus format
   * @param[io] asyncResp     Response object that will be returned to client
   *
   * @return true if give IP is valid and has been sent do D-Bus, false
   * otherwise
   */
  void changeIPv4Origin(const std::string &ifaceId, int ipIdx,
                        const std::string &ipHash, const std::string &newValue,
                        const std::string &newValueDbus,
                        const std::shared_ptr<AsyncResp> &asyncResp) {
    auto callback =
        [ asyncResp, ipIdx{std::move(ipIdx)},
          newValue{std::move(newValue)} ](const boost::system::error_code ec) {
      if (ec) {
        messages::addMessageToJson(
            asyncResp->res.json_value, messages::internalError(),
            "/IPv4Addresses/" + std::to_string(ipIdx) + "/AddressOrigin");
      } else {
        asyncResp->res.json_value["IPv4Addresses"][ipIdx]["AddressOrigin"] =
            newValue;
      }
    };

    crow::connections::system_bus->async_method_call(
        std::move(callback), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "Origin",
        sdbusplus::message::variant<std::string>(newValueDbus));
  };

  /**
   * @brief Modifies SubnetMask for given IP
   *
   * @param[in] ifaceId      Id of interface whose IP should be modified
   * @param[in] ipIdx        Index of IP in input array that should be modified
   * @param[in] ipHash       DBus Hash id of modified IP
   * @param[in] newValueStr  Mask in dot notation as string
   * @param[in] newValue     Mask as PrefixLength in bitcount
   * @param[io] asyncResp   Response object that will be returned to client
   *
   * @return None
   */
  void changeIPv4SubnetMaskProperty(
      const std::string &ifaceId, int ipIdx, const std::string &ipHash,
      const std::string &newValueStr, uint8_t &newValue,
      const std::shared_ptr<AsyncResp> &asyncResp) {
    auto callback = [
      asyncResp, ipIdx{std::move(ipIdx)}, newValueStr{std::move(newValueStr)}
    ](const boost::system::error_code ec) {
      if (ec) {
        messages::addMessageToJson(
            asyncResp->res.json_value, messages::internalError(),
            "/IPv4Addresses/" + std::to_string(ipIdx) + "/SubnetMask");
      } else {
        asyncResp->res.json_value["IPv4Addresses"][ipIdx]["SubnetMask"] =
            newValueStr;
      }
    };

    crow::connections::system_bus->async_method_call(
        std::move(callback), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "PrefixLength",
        sdbusplus::message::variant<uint8_t>(newValue));
  };

  /**
   * @brief Disables VLAN with given ifaceId
   *
   * @param[in] ifaceId   Id of VLAN interface that should be disabled
   * @param[in] callback  Function that will be called after the operation
   *
   * @return None.
   */
  template <typename CallbackFunc>
  void disableVlan(const std::string &ifaceId, CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        callback, "xyz.openbmc_project.Network",
        std::string("/xyz/openbmc_project/network/") + ifaceId,
        "xyz.openbmc_project.Object.Delete", "Delete");
  };

  /**
   * @brief Sets given HostName of the machine through D-Bus
   *
   * @param[in] newHostname   New name that HostName will be changed to
   * @param[in] callback      Function that will be called after the operation
   *
   * @return None.
   */
  template <typename CallbackFunc>
  void setHostName(const std::string &newHostname, CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        callback, "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/config",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
        sdbusplus::message::variant<std::string>(newHostname));
  };

  /**
   * @brief Deletes given IPv4
   *
   * @param[in] ifaceId     Id of interface whose IP should be deleted
   * @param[in] ipIdx       Index of IP in input array that should be deleted
   * @param[in] ipHash      DBus Hash id of IP that should be deleted
   * @param[io] asyncResp   Response object that will be returned to client
   *
   * @return None
   */
  void deleteIPv4(const std::string &ifaceId, const std::string &ipHash,
                  unsigned int ipIdx,
                  const std::shared_ptr<AsyncResp> &asyncResp) {
    crow::connections::system_bus->async_method_call(
        [ ipIdx{std::move(ipIdx)}, asyncResp{std::move(asyncResp)} ](
            const boost::system::error_code ec) {
          if (ec) {
            messages::addMessageToJson(
                asyncResp->res.json_value, messages::internalError(),
                "/IPv4Addresses/" + std::to_string(ipIdx) + "/");
          } else {
            asyncResp->res.json_value["IPv4Addresses"][ipIdx] = nullptr;
          }
        },
        "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "xyz.openbmc_project.Object.Delete", "Delete");
  }

  /**
   * @brief Creates IPv4 with given data
   *
   * @param[in] ifaceId     Id of interface whose IP should be deleted
   * @param[in] ipIdx       Index of IP in input array that should be deleted
   * @param[in] ipHash      DBus Hash id of IP that should be deleted
   * @param[io] asyncResp   Response object that will be returned to client
   *
   * @return None
   */
  void createIPv4(const std::string &ifaceId, unsigned int ipIdx,
                  uint8_t subnetMask, const std::string &gateway,
                  const std::string &address,
                  const std::shared_ptr<AsyncResp> &asyncResp) {
    auto createIpHandler = [
      ipIdx{std::move(ipIdx)}, asyncResp{std::move(asyncResp)}
    ](const boost::system::error_code ec) {
      if (ec) {
        messages::addMessageToJson(
            asyncResp->res.json_value, messages::internalError(),
            "/IPv4Addresses/" + std::to_string(ipIdx) + "/");
      }
    };

    crow::connections::system_bus->async_method_call(
        std::move(createIpHandler), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId,
        "xyz.openbmc_project.Network.IP.Create", "IP",
        "xyz.openbmc_project.Network.IP.Protocol.IPv4", address, subnetMask,
        gateway);
  }

  /**
   * @brief Translates Address Origin value from D-Bus to Redfish format and
   *        vice-versa
   *
   * @param[in] inputOrigin Input value that should be translated
   * @param[in] isIPv4      True for IPv4 origins, False for IPv6
   * @param[in] isFromDBus  True for DBus->Redfish conversion, false for reverse
   *
   * @return Empty string in case of failure, translated value otherwise
   */
  std::string translateAddressOriginBetweenDBusAndRedfish(
      const std::string *inputOrigin, bool isIPv4, bool isFromDBus) {
    // Invalid pointer
    if (inputOrigin == nullptr) {
      return "";
    }

    static const constexpr unsigned int firstIPv4OnlyIdx = 1;
    static const constexpr unsigned int firstIPv6OnlyIdx = 3;

    std::array<std::pair<const char *, const char *>, 6> translationTable{
        {{"xyz.openbmc_project.Network.IP.AddressOrigin.Static", "Static"},
         {"xyz.openbmc_project.Network.IP.AddressOrigin.DHCP", "DHCP"},
         {"xyz.openbmc_project.Network.IP.AddressOrigin.LinkLocal",
          "IPv4LinkLocal"},
         {"xyz.openbmc_project.Network.IP.AddressOrigin.DHCP", "DHCPv6"},
         {"xyz.openbmc_project.Network.IP.AddressOrigin.LinkLocal",
          "LinkLocal"},
         {"xyz.openbmc_project.Network.IP.AddressOrigin.SLAAC", "SLAAC"}}};

    for (unsigned int i = 0; i < translationTable.size(); i++) {
      // Skip unrelated
      if (isIPv4 && i >= firstIPv6OnlyIdx) break;
      if (!isIPv4 && i >= firstIPv4OnlyIdx && i < firstIPv6OnlyIdx) continue;

      // When translating D-Bus to Redfish compare input to first element
      if (isFromDBus && translationTable[i].first == *inputOrigin)
        return translationTable[i].second;

      // When translating Redfish to D-Bus compare input to second element
      if (!isFromDBus && translationTable[i].second == *inputOrigin)
        return translationTable[i].first;
    }

    // If we are still here, that means that value has not been found
    return "";
  }

  /**
   * Function that retrieves all properties for given Ethernet Interface
   * Object
   * from EntityManager Network Manager
   * @param ethiface_id a eth interface id to query on DBus
   * @param callback a function that shall be called to convert Dbus output
   * into JSON
   */
  template <typename CallbackFunc>
  void getEthernetIfaceData(const std::string &ethiface_id,
                            CallbackFunc &&callback) {
    crow::connections::system_bus->async_method_call(
        [
          this, ethiface_id{std::move(ethiface_id)},
          callback{std::move(callback)}
        ](const boost::system::error_code error_code,
          const GetManagedObjectsType &resp) {

          EthernetInterfaceData eth_data{};
          std::vector<IPv4AddressData> ipv4_data;
          ipv4_data.reserve(MAX_IPV4_ADDRESSES_PER_INTERFACE);

          if (error_code) {
            // Something wrong on DBus, the error_code is not important at
            // this moment, just return success=false, and empty output. Since
            // size of vector may vary depending on information from Network
            // Manager, and empty output could not be treated same way as
            // error.
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

          // Finally make a callback with usefull data
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
                const std::string &iface_id =
                    static_cast<const std::string &>(objpath.first);
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
            res.result(boost::beast::http::status::internal_server_error);
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

    entityPrivileges = {
        {boost::beast::http::verb::get, {{"Login"}}},
        {boost::beast::http::verb::head, {{"Login"}}},
        {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
        {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
  }

 private:
  void handleVlanPatch(const std::string &ifaceId, const nlohmann::json &input,
                       const EthernetInterfaceData &eth_data,
                       const std::shared_ptr<AsyncResp> &asyncResp) {
    if (!input.is_object()) {
      messages::addMessageToJson(
          asyncResp->res.json_value,
          messages::propertyValueTypeError(input.dump(), "VLAN"), "/VLAN");
      return;
    }

    bool inputVlanEnabled;
    uint64_t inputVlanId;
    json_util::Result inputVlanEnabledState = json_util::getBool(
        "VLANEnable", input, inputVlanEnabled,
        static_cast<int>(json_util::MessageSetting::TYPE_ERROR),
        asyncResp->res.json_value, std::string("/VLAN/VLANEnable"));
    json_util::Result inputVlanIdState = json_util::getUnsigned(
        "VLANId", input, inputVlanId,
        static_cast<int>(json_util::MessageSetting::TYPE_ERROR),
        asyncResp->res.json_value, std::string("/VLAN/VLANId"));
    bool inputInvalid = false;

    // Do not proceed if fields in VLAN object were of wrong type
    if (inputVlanEnabledState == json_util::Result::WRONG_TYPE ||
        inputVlanIdState == json_util::Result::WRONG_TYPE) {
      return;
    }

    // Verify input
    if (eth_data.vlan_id == nullptr) {
      // VLAN is currently disabled. User can only create/enable it. Change of
      // VLANId is prohibited, and disable request (VLANEnabled == false) will
      // not have any effect.
      if (inputVlanEnabledState == json_util::Result::SUCCESS &&
          inputVlanEnabled == true) {
        // Creation requested, user should also provide ID for new VLAN
        if (inputVlanIdState != json_util::Result::SUCCESS) {
          messages::addMessageToJson(asyncResp->res.json_value,
                                     messages::propertyMissing("VLANId"),
                                     "/VLAN");
          inputInvalid = true;
        }
      } else if (inputVlanIdState == json_util::Result::SUCCESS) {
        // VLAN is disabled, but user requested modification. This is not valid.
        messages::addMessageToJson(
            asyncResp->res.json_value,
            messages::actionParameterNotSupported("VLANId", "change VLAN Id"),
            "/VLAN");

        messages::addMessageToJson(asyncResp->res.json_value,
                                   messages::propertyMissing("VLANEnable"),
                                   "/VLAN");

        inputInvalid = true;
      }
    } else {
      // Load actual data into field values if they were not provided
      if (inputVlanEnabledState == json_util::Result::NOT_EXIST) {
        inputVlanEnabled = true;
      }

      if (inputVlanIdState == json_util::Result::NOT_EXIST) {
        inputVlanId = *eth_data.vlan_id;
      }
    }

    // Do not proceed if input has not been valid
    if (inputInvalid) {
      return;
    }

    auto vlanEnabledAfterOperation =
        [asyncResp](const boost::system::error_code ec) {
          if (ec) {
            messages::addMessageToJson(asyncResp->res.json_value,
                                       messages::internalError(), "/VLAN");
          } else {
            asyncResp->res.json_value["VLAN"]["VLANEnable"] = true;
          }
        };

    if (eth_data.vlan_id == nullptr) {
      if (inputVlanEnabled == true) {
        ethernet_provider.createVlan(ifaceId, inputVlanId,
                                     std::move(vlanEnabledAfterOperation));
        asyncResp->res.json_value["VLAN"]["VLANId"] = inputVlanId;
      }
    } else {
      // VLAN is configured on the interface
      if (inputVlanEnabled == true && inputVlanId != *eth_data.vlan_id) {
        // Change VLAN Id
        asyncResp->res.json_value["VLAN"]["VLANId"] = inputVlanId;
        ethernet_provider.changeVlanId(ifaceId,
                                       static_cast<uint32_t>(inputVlanId),
                                       std::move(vlanEnabledAfterOperation));
      } else if (inputVlanEnabled == false) {
        // Disable VLAN
        ethernet_provider.disableVlan(
            ifaceId, [asyncResp](const boost::system::error_code ec) {
              if (ec) {
                messages::addMessageToJson(asyncResp->res.json_value,
                                           messages::internalError(), "/VLAN");
              } else {
                asyncResp->res.json_value["VLAN"]["VLANEnable"] = false;
              }
            });
      }
    }
  }

  void handleHostnamePatch(const nlohmann::json &input,
                           const EthernetInterfaceData &eth_data,
                           const std::shared_ptr<AsyncResp> &asyncResp) {
    if (input.is_string()) {
      std::string newHostname = input.get<std::string>();

      if (eth_data.hostname == nullptr || newHostname != *eth_data.hostname) {
        // Change hostname
        ethernet_provider.setHostName(
            newHostname,
            [asyncResp, newHostname](const boost::system::error_code ec) {
              if (ec) {
                messages::addMessageToJson(asyncResp->res.json_value,
                                           messages::internalError(),
                                           "/HostName");
              } else {
                asyncResp->res.json_value["HostName"] = newHostname;
              }
            });
      }
    } else {
      messages::addMessageToJson(
          asyncResp->res.json_value,
          messages::propertyValueTypeError(input.dump(), "HostName"),
          "/HostName");
    }
  }

  void handleIPv4Patch(const std::string &ifaceId, const nlohmann::json &input,
                       const std::vector<IPv4AddressData> &ipv4_data,
                       const std::shared_ptr<AsyncResp> &asyncResp) {
    if (!input.is_array()) {
      messages::addMessageToJson(
          asyncResp->res.json_value,
          messages::propertyValueTypeError(input.dump(), "IPv4Addresses"),
          "/IPv4Addresses");
      return;
    }

    // According to Redfish PATCH definition, size must be at least equal
    if (input.size() < ipv4_data.size()) {
      // TODO(kkowalsk) This should be a message indicating that not enough
      // data has been provided
      messages::addMessageToJson(asyncResp->res.json_value,
                                 messages::internalError(), "/IPv4Addresses");
      return;
    }

    json_util::Result addressFieldState;
    json_util::Result subnetMaskFieldState;
    json_util::Result addressOriginFieldState;
    json_util::Result gatewayFieldState;
    const std::string *addressFieldValue;
    const std::string *subnetMaskFieldValue;
    const std::string *addressOriginFieldValue = nullptr;
    const std::string *gatewayFieldValue;
    uint8_t subnetMaskAsPrefixLength;
    std::string addressOriginInDBusFormat;

    bool errorDetected = false;
    for (unsigned int entryIdx = 0; entryIdx < input.size(); entryIdx++) {
      // Check that entry is not of some unexpected type
      if (!input[entryIdx].is_object() && !input[entryIdx].is_null()) {
        // Invalid object type
        messages::addMessageToJson(
            asyncResp->res.json_value,
            messages::propertyValueTypeError(input[entryIdx].dump(),
                                             "IPv4Address"),
            "/IPv4Addresses/" + std::to_string(entryIdx));

        continue;
      }

      // Try to load fields
      addressFieldState = json_util::getString(
          "Address", input[entryIdx], addressFieldValue,
          static_cast<uint8_t>(json_util::MessageSetting::TYPE_ERROR),
          asyncResp->res.json_value,
          "/IPv4Addresses/" + std::to_string(entryIdx) + "/Address");
      subnetMaskFieldState = json_util::getString(
          "SubnetMask", input[entryIdx], subnetMaskFieldValue,
          static_cast<uint8_t>(json_util::MessageSetting::TYPE_ERROR),
          asyncResp->res.json_value,
          "/IPv4Addresses/" + std::to_string(entryIdx) + "/SubnetMask");
      addressOriginFieldState = json_util::getString(
          "AddressOrigin", input[entryIdx], addressOriginFieldValue,
          static_cast<uint8_t>(json_util::MessageSetting::TYPE_ERROR),
          asyncResp->res.json_value,
          "/IPv4Addresses/" + std::to_string(entryIdx) + "/AddressOrigin");
      gatewayFieldState = json_util::getString(
          "Gateway", input[entryIdx], gatewayFieldValue,
          static_cast<uint8_t>(json_util::MessageSetting::TYPE_ERROR),
          asyncResp->res.json_value,
          "/IPv4Addresses/" + std::to_string(entryIdx) + "/Gateway");

      if (addressFieldState == json_util::Result::WRONG_TYPE ||
          subnetMaskFieldState == json_util::Result::WRONG_TYPE ||
          addressOriginFieldState == json_util::Result::WRONG_TYPE ||
          gatewayFieldState == json_util::Result::WRONG_TYPE) {
        return;
      }

      if (addressFieldState == json_util::Result::SUCCESS &&
          !ethernet_provider.ipv4VerifyIpAndGetBitcount(*addressFieldValue)) {
        errorDetected = true;
        messages::addMessageToJson(
            asyncResp->res.json_value,
            messages::propertyValueFormatError(*addressFieldValue, "Address"),
            "/IPv4Addresses/" + std::to_string(entryIdx) + "/Address");
      }

      if (subnetMaskFieldState == json_util::Result::SUCCESS &&
          !ethernet_provider.ipv4VerifyIpAndGetBitcount(
              *subnetMaskFieldValue, &subnetMaskAsPrefixLength)) {
        errorDetected = true;
        messages::addMessageToJson(
            asyncResp->res.json_value,
            messages::propertyValueFormatError(*subnetMaskFieldValue,
                                               "SubnetMask"),
            "/IPv4Addresses/" + std::to_string(entryIdx) + "/SubnetMask");
      }

      // Get Address origin in proper format
      addressOriginInDBusFormat =
          ethernet_provider.translateAddressOriginBetweenDBusAndRedfish(
              addressOriginFieldValue, true, false);

      if (addressOriginFieldState == json_util::Result::SUCCESS &&
          addressOriginInDBusFormat.empty()) {
        errorDetected = true;
        messages::addMessageToJson(
            asyncResp->res.json_value,
            messages::propertyValueNotInList(*addressOriginFieldValue,
                                             "AddressOrigin"),
            "/IPv4Addresses/" + std::to_string(entryIdx) + "/AddressOrigin");
      }

      if (gatewayFieldState == json_util::Result::SUCCESS &&
          !ethernet_provider.ipv4VerifyIpAndGetBitcount(*gatewayFieldValue)) {
        errorDetected = true;
        messages::addMessageToJson(
            asyncResp->res.json_value,
            messages::propertyValueFormatError(*gatewayFieldValue, "Gateway"),
            "/IPv4Addresses/" + std::to_string(entryIdx) + "/Gateway");
      }

      // If any error occured do not proceed with current entry, but do not
      // end loop
      if (errorDetected) {
        errorDetected = false;
        continue;
      }

      if (entryIdx >= ipv4_data.size()) {
        asyncResp->res.json_value["IPv4Addresses"][entryIdx] = input[entryIdx];

        // Verify that all field were provided
        if (addressFieldState == json_util::Result::NOT_EXIST) {
          errorDetected = true;
          messages::addMessageToJson(
              asyncResp->res.json_value, messages::propertyMissing("Address"),
              "/IPv4Addresses/" + std::to_string(entryIdx) + "/Address");
        }

        if (subnetMaskFieldState == json_util::Result::NOT_EXIST) {
          errorDetected = true;
          messages::addMessageToJson(
              asyncResp->res.json_value,
              messages::propertyMissing("SubnetMask"),
              "/IPv4Addresses/" + std::to_string(entryIdx) + "/SubnetMask");
        }

        if (addressOriginFieldState == json_util::Result::NOT_EXIST) {
          errorDetected = true;
          messages::addMessageToJson(
              asyncResp->res.json_value,
              messages::propertyMissing("AddressOrigin"),
              "/IPv4Addresses/" + std::to_string(entryIdx) + "/AddressOrigin");
        }

        if (gatewayFieldState == json_util::Result::NOT_EXIST) {
          errorDetected = true;
          messages::addMessageToJson(
              asyncResp->res.json_value, messages::propertyMissing("Gateway"),
              "/IPv4Addresses/" + std::to_string(entryIdx) + "/Gateway");
        }

        // If any error occured do not proceed with current entry, but do not
        // end loop
        if (errorDetected) {
          errorDetected = false;
          continue;
        }

        // Create IPv4 with provided data
        ethernet_provider.createIPv4(
            ifaceId, entryIdx, subnetMaskAsPrefixLength, *gatewayFieldValue,
            *addressFieldValue, asyncResp);
      } else {
        // Existing object that should be modified/deleted/remain unchanged
        if (input[entryIdx].is_null()) {
          // Object should be deleted
          ethernet_provider.deleteIPv4(ifaceId, ipv4_data[entryIdx].id,
                                       entryIdx, asyncResp);
        } else if (input[entryIdx].is_object()) {
          if (input[entryIdx].size() == 0) {
            // Object shall remain unchanged
            continue;
          }

          // Apply changes
          if (addressFieldState == json_util::Result::SUCCESS &&
              ipv4_data[entryIdx].address != nullptr &&
              *ipv4_data[entryIdx].address != *addressFieldValue) {
            ethernet_provider.changeIPv4AddressProperty(
                ifaceId, entryIdx, ipv4_data[entryIdx].id, "Address",
                *addressFieldValue, asyncResp);
          }

          if (subnetMaskFieldState == json_util::Result::SUCCESS &&
              ipv4_data[entryIdx].netmask != *subnetMaskFieldValue) {
            ethernet_provider.changeIPv4SubnetMaskProperty(
                ifaceId, entryIdx, ipv4_data[entryIdx].id,
                *subnetMaskFieldValue, subnetMaskAsPrefixLength, asyncResp);
          }

          if (addressOriginFieldState == json_util::Result::SUCCESS &&
              ipv4_data[entryIdx].origin != *addressFieldValue) {
            ethernet_provider.changeIPv4Origin(
                ifaceId, entryIdx, ipv4_data[entryIdx].id,
                *addressOriginFieldValue, addressOriginInDBusFormat, asyncResp);
          }

          if (gatewayFieldState == json_util::Result::SUCCESS &&
              ipv4_data[entryIdx].gateway != nullptr &&
              *ipv4_data[entryIdx].gateway != *gatewayFieldValue) {
            ethernet_provider.changeIPv4AddressProperty(
                ifaceId, entryIdx, ipv4_data[entryIdx].id, "Gateway",
                *gatewayFieldValue, asyncResp);
          }
        }
      }
    }
  }

  nlohmann::json parseInterfaceData(
      const std::string &iface_id, const EthernetInterfaceData &eth_data,
      const std::vector<IPv4AddressData> &ipv4_data) {
    // Copy JSON object to avoid race condition
    nlohmann::json json_response(Node::json);

    // Fill out obvious data...
    json_response["Id"] = iface_id;
    json_response["@odata.id"] =
        "/redfish/v1/Managers/openbmc/EthernetInterfaces/" + iface_id;

    // ... then the one from DBus, regarding eth iface...
    if (eth_data.speed != nullptr) json_response["SpeedMbps"] = *eth_data.speed;

    if (eth_data.mac_address != nullptr)
      json_response["MACAddress"] = *eth_data.mac_address;

    if (eth_data.hostname != nullptr)
      json_response["HostName"] = *eth_data.hostname;

    if (eth_data.vlan_id != nullptr) {
      nlohmann::json &vlanObj = json_response["VLAN"];
      vlanObj["VLANEnable"] = true;
      vlanObj["VLANId"] = *eth_data.vlan_id;
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

          ipv4_array.push_back(std::move(json_ipv4));
        }
      }
      json_response["IPv4Addresses"] = std::move(ipv4_array);
    }

    return json_response;
  }

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
      res.result(boost::beast::http::status::internal_server_error);
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
            res.json_value = parseInterfaceData(iface_id, eth_data, ipv4_data);
          } else {
            // ... otherwise return error
            // TODO(Pawel)consider distinguish between non existing object, and
            // other errors
            res.result(boost::beast::http::status::not_found);
          }
          res.end();
        });
  }

  void doPatch(crow::response &res, const crow::request &req,
               const std::vector<std::string> &params) override {
    // TODO(Pawel) this shall be parametrized call (two params) to get
    // EthernetInterfaces for any Manager, not only hardcoded 'openbmc'.
    // Check if there is required param, truly entering this shall be
    // impossible.
    if (params.size() != 1) {
      res.result(boost::beast::http::status::internal_server_error);
      res.end();
      return;
    }

    const std::string &iface_id = params[0];

    nlohmann::json patchReq;

    if (!json_util::processJsonFromRequest(res, req, patchReq)) {
      return;
    }

    // Get single eth interface data, and call the below callback for JSON
    // preparation
    ethernet_provider.getEthernetIfaceData(
        iface_id,
        [&, iface_id, patchReq = std::move(patchReq) ](
            const bool &success, const EthernetInterfaceData &eth_data,
            const std::vector<IPv4AddressData> &ipv4_data) {
          if (!success) {
            // ... otherwise return error
            // TODO(Pawel)consider distinguish between non existing object, and
            // other errors
            res.result(boost::beast::http::status::not_found);
            res.end();

            return;
          }

          res.json_value = parseInterfaceData(iface_id, eth_data, ipv4_data);

          std::shared_ptr<AsyncResp> asyncResp =
              std::make_shared<AsyncResp>(res);

          for (auto propertyIt = patchReq.begin(); propertyIt != patchReq.end();
               ++propertyIt) {
            if (propertyIt.key() == "VLAN") {
              handleVlanPatch(iface_id, propertyIt.value(), eth_data,
                              asyncResp);
            } else if (propertyIt.key() == "HostName") {
              handleHostnamePatch(propertyIt.value(), eth_data, asyncResp);
            } else if (propertyIt.key() == "IPv4Addresses") {
              handleIPv4Patch(iface_id, propertyIt.value(), ipv4_data,
                              asyncResp);
            } else if (propertyIt.key() == "IPv6Addresses") {
              // TODO(kkowalsk) IPv6 Not supported on D-Bus yet
              messages::addMessageToJsonRoot(
                  res.json_value,
                  messages::propertyNotWritable(propertyIt.key()));
            } else {
              auto fieldInJsonIt = res.json_value.find(propertyIt.key());

              if (fieldInJsonIt == res.json_value.end()) {
                // Field not in scope of defined fields
                messages::addMessageToJsonRoot(
                    res.json_value,
                    messages::propertyUnknown(propertyIt.key()));
              } else if (*fieldInJsonIt != *propertyIt) {
                // User attempted to modify non-writable field
                messages::addMessageToJsonRoot(
                    res.json_value,
                    messages::propertyNotWritable(propertyIt.key()));
              }
            }
          }
        });
  }

  // Ethernet Provider object
  // TODO(Pawel) consider move it to singleton
  OnDemandEthernetProvider ethernet_provider;
};

}  // namespace redfish
