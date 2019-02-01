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

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <error_messages.hpp>
#include <node.hpp>
#include <optional>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

/**
 * DBus types primitives for several generic DBus interfaces
 * TODO(Pawel) consider move this to separate file into boost::dbus
 */
using PropertiesMapType = boost::container::flat_map<
    std::string, std::variant<std::string, bool, uint8_t, int16_t, uint16_t,
                              int32_t, uint32_t, int64_t, uint64_t, double>>;

using GetManagedObjects = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<
        std::string,
        boost::container::flat_map<
            std::string, sdbusplus::message::variant<
                             std::string, bool, uint8_t, int16_t, uint16_t,
                             int32_t, uint32_t, int64_t, uint64_t, double,
                             std::vector<std::string>>>>>>>;

enum class LinkType
{
    Local,
    Global
};

/**
 * Structure for keeping IPv4 data required by Redfish
 */
struct IPv4AddressData
{
    std::string id;
    std::string address;
    std::string domain;
    std::string gateway;
    std::string netmask;
    std::string origin;
    LinkType linktype;

    bool operator<(const IPv4AddressData &obj) const
    {
        return id < obj.id;
    }
};

/**
 * Structure for keeping basic single Ethernet Interface information
 * available from DBus
 */
struct EthernetInterfaceData
{
    uint32_t speed;
    bool auto_neg;
    std::string hostname;
    std::string default_gateway;
    std::string mac_address;
    std::optional<uint32_t> vlan_id;
    std::vector<std::string> nameservers;
};

// Helper function that changes bits netmask notation (i.e. /24)
// into full dot notation
inline std::string getNetmask(unsigned int bits)
{
    uint32_t value = 0xffffffff << (32 - bits);
    std::string netmask = std::to_string((value >> 24) & 0xff) + "." +
                          std::to_string((value >> 16) & 0xff) + "." +
                          std::to_string((value >> 8) & 0xff) + "." +
                          std::to_string(value & 0xff);
    return netmask;
}

inline std::string
    translateAddressOriginDbusToRedfish(const std::string &inputOrigin,
                                        bool isIPv4)
{
    if (inputOrigin == "xyz.openbmc_project.Network.IP.AddressOrigin.Static")
    {
        return "Static";
    }
    if (inputOrigin == "xyz.openbmc_project.Network.IP.AddressOrigin.LinkLocal")
    {
        if (isIPv4)
        {
            return "IPv4LinkLocal";
        }
        else
        {
            return "LinkLocal";
        }
    }
    if (inputOrigin == "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP")
    {
        if (isIPv4)
        {
            return "DHCP";
        }
        else
        {
            return "DHCPv6";
        }
    }
    if (inputOrigin == "xyz.openbmc_project.Network.IP.AddressOrigin.SLAAC")
    {
        return "SLAAC";
    }
    return "";
}

inline std::string
    translateAddressOriginRedfishToDbus(const std::string &inputOrigin)
{
    if (inputOrigin == "Static")
    {
        return "xyz.openbmc_project.Network.IP.AddressOrigin.Static";
    }
    if (inputOrigin == "DHCP" || inputOrigin == "DHCPv6")
    {
        return "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP";
    }
    if (inputOrigin == "IPv4LinkLocal" || inputOrigin == "LinkLocal")
    {
        return "xyz.openbmc_project.Network.IP.AddressOrigin.LinkLocal";
    }
    if (inputOrigin == "SLAAC")
    {
        return "xyz.openbmc_project.Network.IP.AddressOrigin.SLAAC";
    }
    return "";
}

inline void extractEthernetInterfaceData(const std::string &ethiface_id,
                                         const GetManagedObjects &dbus_data,
                                         EthernetInterfaceData &ethData)
{
    for (const auto &objpath : dbus_data)
    {
        for (const auto &ifacePair : objpath.second)
        {
            if (objpath.first == "/xyz/openbmc_project/network/" + ethiface_id)
            {
                if (ifacePair.first == "xyz.openbmc_project.Network.MACAddress")
                {
                    for (const auto &propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "MACAddress")
                        {
                            const std::string *mac =
                                std::get_if<std::string>(&propertyPair.second);
                            if (mac != nullptr)
                            {
                                ethData.mac_address = *mac;
                            }
                        }
                    }
                }
                else if (ifacePair.first == "xyz.openbmc_project.Network.VLAN")
                {
                    for (const auto &propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "Id")
                        {
                            const uint32_t *id =
                                std::get_if<uint32_t>(&propertyPair.second);
                            if (id != nullptr)
                            {
                                ethData.vlan_id = *id;
                            }
                        }
                    }
                }
                else if (ifacePair.first ==
                         "xyz.openbmc_project.Network.EthernetInterface")
                {
                    for (const auto &propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "AutoNeg")
                        {
                            const bool *auto_neg =
                                std::get_if<bool>(&propertyPair.second);
                            if (auto_neg != nullptr)
                            {
                                ethData.auto_neg = *auto_neg;
                            }
                        }
                        else if (propertyPair.first == "Speed")
                        {
                            const uint32_t *speed =
                                std::get_if<uint32_t>(&propertyPair.second);
                            if (speed != nullptr)
                            {
                                ethData.speed = *speed;
                            }
                        }
                        else if (propertyPair.first == "NameServers")
                        {
                            const std::vector<std::string> *nameservers =
                                sdbusplus::message::variant_ns::get_if<
                                    std::vector<std::string>>(
                                    &propertyPair.second);
                            if (nameservers != nullptr)
                            {
                                ethData.nameservers = std::move(*nameservers);
                            }
                        }
                    }
                }
            }
            // System configuration shows up in the global namespace, so no need
            // to check eth number
            if (ifacePair.first ==
                "xyz.openbmc_project.Network.SystemConfiguration")
            {
                for (const auto &propertyPair : ifacePair.second)
                {
                    if (propertyPair.first == "HostName")
                    {
                        const std::string *hostname =
                            sdbusplus::message::variant_ns::get_if<std::string>(
                                &propertyPair.second);
                        if (hostname != nullptr)
                        {
                            ethData.hostname = *hostname;
                        }
                    }
                    else if (propertyPair.first == "DefaultGateway")
                    {
                        const std::string *defaultGateway =
                            sdbusplus::message::variant_ns::get_if<std::string>(
                                &propertyPair.second);
                        if (defaultGateway != nullptr)
                        {
                            ethData.default_gateway = *defaultGateway;
                        }
                    }
                }
            }
        }
    }
}

// Helper function that extracts data for single ethernet ipv4 address
inline void
    extractIPData(const std::string &ethiface_id,
                  const GetManagedObjects &dbus_data,
                  boost::container::flat_set<IPv4AddressData> &ipv4_config)
{
    const std::string ipv4PathStart =
        "/xyz/openbmc_project/network/" + ethiface_id + "/ipv4/";

    // Since there might be several IPv4 configurations aligned with
    // single ethernet interface, loop over all of them
    for (const auto &objpath : dbus_data)
    {
        // Check if proper pattern for object path appears
        if (boost::starts_with(objpath.first.str, ipv4PathStart))
        {
            for (auto &interface : objpath.second)
            {
                if (interface.first == "xyz.openbmc_project.Network.IP")
                {
                    // Instance IPv4AddressData structure, and set as
                    // appropriate
                    std::pair<
                        boost::container::flat_set<IPv4AddressData>::iterator,
                        bool>
                        it = ipv4_config.insert(
                            {objpath.first.str.substr(ipv4PathStart.size())});
                    IPv4AddressData &ipv4_address = *it.first;
                    for (auto &property : interface.second)
                    {
                        if (property.first == "Address")
                        {
                            const std::string *address =
                                std::get_if<std::string>(&property.second);
                            if (address != nullptr)
                            {
                                ipv4_address.address = *address;
                            }
                        }
                        else if (property.first == "Gateway")
                        {
                            const std::string *gateway =
                                std::get_if<std::string>(&property.second);
                            if (gateway != nullptr)
                            {
                                ipv4_address.gateway = *gateway;
                            }
                        }
                        else if (property.first == "Origin")
                        {
                            const std::string *origin =
                                std::get_if<std::string>(&property.second);
                            if (origin != nullptr)
                            {
                                ipv4_address.origin =
                                    translateAddressOriginDbusToRedfish(*origin,
                                                                        true);
                            }
                        }
                        else if (property.first == "PrefixLength")
                        {
                            const uint8_t *mask =
                                std::get_if<uint8_t>(&property.second);
                            if (mask != nullptr)
                            {
                                // convert it to the string
                                ipv4_address.netmask = getNetmask(*mask);
                            }
                        }
                        else
                        {
                            BMCWEB_LOG_ERROR
                                << "Got extra property: " << property.first
                                << " on the " << objpath.first.str << " object";
                        }
                    }
                    // Check if given address is local, or global
                    ipv4_address.linktype =
                        boost::starts_with(ipv4_address.address, "169.254.")
                            ? LinkType::Global
                            : LinkType::Local;
                }
            }
        }
    }
}

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
                  CallbackFunc &&callback)
{
    crow::connections::systemBus->async_method_call(
        callback, "xyz.openbmc_project.Network",
        std::string("/xyz/openbmc_project/network/") + ifaceId,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.VLAN", "Id",
        std::variant<uint32_t>(inputVlanId));
}

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
inline bool ipv4VerifyIpAndGetBitcount(const std::string &ip,
                                       uint8_t *bits = nullptr)
{
    std::vector<std::string> bytesInMask;

    boost::split(bytesInMask, ip, boost::is_any_of("."));

    static const constexpr int ipV4AddressSectionsCount = 4;
    if (bytesInMask.size() != ipV4AddressSectionsCount)
    {
        return false;
    }

    if (bits != nullptr)
    {
        *bits = 0;
    }

    char *endPtr;
    long previousValue = 255;
    bool firstZeroInByteHit;
    for (const std::string &byte : bytesInMask)
    {
        if (byte.empty())
        {
            return false;
        }

        // Use strtol instead of stroi to avoid exceptions
        long value = std::strtol(byte.c_str(), &endPtr, 10);

        // endPtr should point to the end of the string, otherwise given string
        // is not 100% number
        if (*endPtr != '\0')
        {
            return false;
        }

        // Value should be contained in byte
        if (value < 0 || value > 255)
        {
            return false;
        }

        if (bits != nullptr)
        {
            // Mask has to be continuous between bytes
            if (previousValue != 255 && value != 0)
            {
                return false;
            }

            // Mask has to be continuous inside bytes
            firstZeroInByteHit = false;

            // Count bits
            for (int bitIdx = 7; bitIdx >= 0; bitIdx--)
            {
                if (value & (1 << bitIdx))
                {
                    if (firstZeroInByteHit)
                    {
                        // Continuity not preserved
                        return false;
                    }
                    else
                    {
                        (*bits)++;
                    }
                }
                else
                {
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
inline void changeIPv4AddressProperty(
    const std::string &ifaceId, int ipIdx, const std::string &ipHash,
    const std::string &name, const std::string &newValue,
    const std::shared_ptr<AsyncResp> asyncResp)
{
    auto callback = [asyncResp, ipIdx, name{std::string(name)},
                     newValue{std::move(newValue)}](
                        const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
        }
        else
        {
            asyncResp->res.jsonValue["IPv4Addresses"][ipIdx][name] = newValue;
        }
    };

    crow::connections::systemBus->async_method_call(
        std::move(callback), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", name,
        std::variant<std::string>(newValue));
}

/**
 * @brief Changes IPv4 address origin property
 *
 * @param[in] ifaceId       Id of interface whose IP should be modified
 * @param[in] ipIdx         Index of IP in input array that should be
 * modified
 * @param[in] ipHash        DBus Hash id of modified IP
 * @param[in] newValue      New value in Redfish format
 * @param[in] newValueDbus  New value in D-Bus format
 * @param[io] asyncResp     Response object that will be returned to client
 *
 * @return true if give IP is valid and has been sent do D-Bus, false
 * otherwise
 */
inline void changeIPv4Origin(const std::string &ifaceId, int ipIdx,
                             const std::string &ipHash,
                             const std::string &newValue,
                             const std::string &newValueDbus,
                             const std::shared_ptr<AsyncResp> asyncResp)
{
    auto callback = [asyncResp, ipIdx, newValue{std::move(newValue)}](
                        const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
        }
        else
        {
            asyncResp->res.jsonValue["IPv4Addresses"][ipIdx]["AddressOrigin"] =
                newValue;
        }
    };

    crow::connections::systemBus->async_method_call(
        std::move(callback), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "Origin",
        std::variant<std::string>(newValueDbus));
}

/**
 * @brief Modifies SubnetMask for given IP
 *
 * @param[in] ifaceId      Id of interface whose IP should be modified
 * @param[in] ipIdx        Index of IP in input array that should be
 * modified
 * @param[in] ipHash       DBus Hash id of modified IP
 * @param[in] newValueStr  Mask in dot notation as string
 * @param[in] newValue     Mask as PrefixLength in bitcount
 * @param[io] asyncResp   Response object that will be returned to client
 *
 * @return None
 */
inline void changeIPv4SubnetMaskProperty(const std::string &ifaceId, int ipIdx,
                                         const std::string &ipHash,
                                         const std::string &newValueStr,
                                         uint8_t &newValue,
                                         std::shared_ptr<AsyncResp> asyncResp)
{
    auto callback = [asyncResp, ipIdx, newValueStr{std::move(newValueStr)}](
                        const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
        }
        else
        {
            asyncResp->res.jsonValue["IPv4Addresses"][ipIdx]["SubnetMask"] =
                newValueStr;
        }
    };

    crow::connections::systemBus->async_method_call(
        std::move(callback), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "PrefixLength",
        std::variant<uint8_t>(newValue));
}

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
inline void deleteIPv4(const std::string &ifaceId, const std::string &ipHash,
                       unsigned int ipIdx,
                       const std::shared_ptr<AsyncResp> asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [ipIdx, asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
            else
            {
                asyncResp->res.jsonValue["IPv4Addresses"][ipIdx] = nullptr;
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
inline void createIPv4(const std::string &ifaceId, unsigned int ipIdx,
                       uint8_t subnetMask, const std::string &gateway,
                       const std::string &address,
                       std::shared_ptr<AsyncResp> asyncResp)
{
    auto createIpHandler = [ipIdx,
                            asyncResp](const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
        }
    };

    crow::connections::systemBus->async_method_call(
        std::move(createIpHandler), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId,
        "xyz.openbmc_project.Network.IP.Create", "IP",
        "xyz.openbmc_project.Network.IP.Protocol.IPv4", address, subnetMask,
        gateway);
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
                          CallbackFunc &&callback)
{
    crow::connections::systemBus->async_method_call(
        [ethiface_id{std::string{ethiface_id}}, callback{std::move(callback)}](
            const boost::system::error_code error_code,
            const GetManagedObjects &resp) {
            EthernetInterfaceData ethData{};
            boost::container::flat_set<IPv4AddressData> ipv4Data;

            if (error_code)
            {
                callback(false, ethData, ipv4Data);
                return;
            }

            extractEthernetInterfaceData(ethiface_id, resp, ethData);
            extractIPData(ethiface_id, resp, ipv4Data);

            // Fix global GW
            for (IPv4AddressData &ipv4 : ipv4Data)
            {
                if ((ipv4.linktype == LinkType::Global) &&
                    (ipv4.gateway == "0.0.0.0"))
                {
                    ipv4.gateway = ethData.default_gateway;
                }
            }

            // Finally make a callback with usefull data
            callback(true, ethData, ipv4Data);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
};

/**
 * Function that retrieves all Ethernet Interfaces available through Network
 * Manager
 * @param callback a function that shall be called to convert Dbus output
 * into JSON.
 */
template <typename CallbackFunc>
void getEthernetIfaceList(CallbackFunc &&callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code error_code,
            GetManagedObjects &resp) {
            // Callback requires vector<string> to retrieve all available
            // ethernet interfaces
            std::vector<std::string> iface_list;
            iface_list.reserve(resp.size());
            if (error_code)
            {
                callback(false, iface_list);
                return;
            }

            // Iterate over all retrieved ObjectPaths.
            for (const auto &objpath : resp)
            {
                // And all interfaces available for certain ObjectPath.
                for (const auto &interface : objpath.second)
                {
                    // If interface is
                    // xyz.openbmc_project.Network.EthernetInterface, this is
                    // what we're looking for.
                    if (interface.first ==
                        "xyz.openbmc_project.Network.EthernetInterface")
                    {
                        // Cut out everyting until last "/", ...
                        const std::string &iface_id = objpath.first.str;
                        std::size_t last_pos = iface_id.rfind("/");
                        if (last_pos != std::string::npos)
                        {
                            // and put it into output vector.
                            iface_list.emplace_back(
                                iface_id.substr(last_pos + 1));
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

/**
 * EthernetCollection derived class for delivering Ethernet Collection Schema
 */
class EthernetCollection : public Node
{
  public:
    template <typename CrowApp>
    EthernetCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/EthernetInterfaces/")
    {
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] =
            "#EthernetInterfaceCollection.EthernetInterfaceCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#EthernetInterfaceCollection.EthernetInterfaceCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/EthernetInterfaces";
        res.jsonValue["Name"] = "Ethernet Network Interface Collection";
        res.jsonValue["Description"] =
            "Collection of EthernetInterfaces for this Manager";

        // Get eth interface list, and call the below callback for JSON
        // preparation
        getEthernetIfaceList(
            [&res](const bool &success,
                   const std::vector<std::string> &iface_list) {
                if (!success)
                {
                    messages::internalError(res);
                    res.end();
                    return;
                }

                nlohmann::json &iface_array = res.jsonValue["Members"];
                iface_array = nlohmann::json::array();
                for (const std::string &iface_item : iface_list)
                {
                    iface_array.push_back(
                        {{"@odata.id",
                          "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                              iface_item}});
                }

                res.jsonValue["Members@odata.count"] = iface_array.size();
                res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/EthernetInterfaces";
                res.end();
            });
    }
};

/**
 * EthernetInterface derived class for delivering Ethernet Schema
 */
class EthernetInterface : public Node
{
  public:
    /*
     * Default Constructor
     */
    template <typename CrowApp>
    EthernetInterface(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

    // TODO(kkowalsk) Find a suitable class/namespace for this
    static void handleVlanPatch(const std::string &ifaceId, bool vlanEnable,
                                uint64_t vlanId,
                                const EthernetInterfaceData &ethData,
                                const std::shared_ptr<AsyncResp> asyncResp)
    {
        if (!ethData.vlan_id)
        {
            // This interface is not a VLAN. Cannot do anything with it
            // TODO(kkowalsk) Change this message
            messages::propertyNotWritable(asyncResp->res, "VLANEnable");

            return;
        }

        // VLAN is configured on the interface
        if (vlanEnable == true)
        {
            // Change VLAN Id
            asyncResp->res.jsonValue["VLANId"] = vlanId;
            auto callback = [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                }
                else
                {
                    asyncResp->res.jsonValue["VLANEnable"] = true;
                }
            };
            crow::connections::systemBus->async_method_call(
                std::move(callback), "xyz.openbmc_project.Network",
                "/xyz/openbmc_project/network/" + ifaceId,
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Network.VLAN", "Id",
                std::variant<uint32_t>(vlanId));
        }
        else
        {
            auto callback = [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["VLANEnable"] = false;
            };

            crow::connections::systemBus->async_method_call(
                std::move(callback), "xyz.openbmc_project.Network",
                "/xyz/openbmc_project/network/" + ifaceId,
                "xyz.openbmc_project.Object.Delete", "Delete");
        }
    }

  private:
    void handleHostnamePatch(const std::string &hostname,
                             const std::shared_ptr<AsyncResp> asyncResp)
    {
        asyncResp->res.jsonValue["HostName"] = hostname;
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/config",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
            std::variant<std::string>(hostname));
    }

    void handleIPv4Patch(
        const std::string &ifaceId, const nlohmann::json &input,
        const boost::container::flat_set<IPv4AddressData> &ipv4Data,
        const std::shared_ptr<AsyncResp> asyncResp)
    {
        if (!input.is_array())
        {
            messages::propertyValueTypeError(asyncResp->res, input.dump(),
                                             "IPv4Addresses");
            return;
        }

        // According to Redfish PATCH definition, size must be at least equal
        if (input.size() < ipv4Data.size())
        {
            messages::propertyValueFormatError(asyncResp->res, input.dump(),
                                               "IPv4Addresses");
            return;
        }

        int entryIdx = 0;
        boost::container::flat_set<IPv4AddressData>::const_iterator thisData =
            ipv4Data.begin();
        for (const nlohmann::json &thisJson : input)
        {
            std::string pathString =
                "IPv4Addresses/" + std::to_string(entryIdx);
            // Check that entry is not of some unexpected type
            if (!thisJson.is_object() && !thisJson.is_null())
            {
                messages::propertyValueTypeError(asyncResp->res,
                                                 thisJson.dump(),
                                                 pathString + "/IPv4Address");

                continue;
            }

            nlohmann::json::const_iterator addressFieldIt =
                thisJson.find("Address");
            const std::string *addressField = nullptr;
            if (addressFieldIt != thisJson.end())
            {
                addressField = addressFieldIt->get_ptr<const std::string *>();
                if (addressField == nullptr)
                {
                    messages::propertyValueFormatError(asyncResp->res,
                                                       addressFieldIt->dump(),
                                                       pathString + "/Address");
                    continue;
                }
                else
                {
                    if (!ipv4VerifyIpAndGetBitcount(*addressField))
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, *addressField,
                            pathString + "/Address");
                        continue;
                    }
                }
            }

            std::optional<uint8_t> prefixLength;
            const std::string *subnetField = nullptr;
            nlohmann::json::const_iterator subnetFieldIt =
                thisJson.find("SubnetMask");
            if (subnetFieldIt != thisJson.end())
            {
                subnetField = subnetFieldIt->get_ptr<const std::string *>();
                if (subnetField == nullptr)
                {
                    messages::propertyValueFormatError(
                        asyncResp->res, *subnetField,
                        pathString + "/SubnetMask");
                    continue;
                }
                else
                {
                    prefixLength = 0;
                    if (!ipv4VerifyIpAndGetBitcount(*subnetField,
                                                    &*prefixLength))
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, *subnetField,
                            pathString + "/SubnetMask");
                        continue;
                    }
                }
            }

            std::string addressOriginInDBusFormat;
            const std::string *addressOriginField = nullptr;
            nlohmann::json::const_iterator addressOriginFieldIt =
                thisJson.find("AddressOrigin");
            if (addressOriginFieldIt != thisJson.end())
            {
                const std::string *addressOriginField =
                    addressOriginFieldIt->get_ptr<const std::string *>();
                if (addressOriginField == nullptr)
                {
                    messages::propertyValueFormatError(
                        asyncResp->res, *addressOriginField,
                        pathString + "/AddressOrigin");
                    continue;
                }
                else
                {
                    // Get Address origin in proper format
                    addressOriginInDBusFormat =
                        translateAddressOriginRedfishToDbus(
                            *addressOriginField);
                    if (addressOriginInDBusFormat.empty())
                    {
                        messages::propertyValueNotInList(
                            asyncResp->res, *addressOriginField,
                            pathString + "/AddressOrigin");
                        continue;
                    }
                }
            }

            nlohmann::json::const_iterator gatewayFieldIt =
                thisJson.find("Gateway");
            const std::string *gatewayField = nullptr;
            if (gatewayFieldIt != thisJson.end())
            {
                const std::string *gatewayField =
                    gatewayFieldIt->get_ptr<const std::string *>();
                if (gatewayField == nullptr ||
                    !ipv4VerifyIpAndGetBitcount(*gatewayField))
                {
                    messages::propertyValueFormatError(
                        asyncResp->res, *gatewayField, pathString + "/Gateway");
                    continue;
                }
            }

            // if a vlan already exists, modify the existing
            if (thisData != ipv4Data.end())
            {
                // Existing object that should be modified/deleted/remain
                // unchanged
                if (thisJson.is_null())
                {
                    auto callback = [entryIdx{std::to_string(entryIdx)},
                                     asyncResp](
                                        const boost::system::error_code ec) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res.jsonValue["IPv4Addresses"][entryIdx] =
                            nullptr;
                    };
                    crow::connections::systemBus->async_method_call(
                        std::move(callback), "xyz.openbmc_project.Network",
                        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" +
                            thisData->id,
                        "xyz.openbmc_project.Object.Delete", "Delete");
                }
                else if (thisJson.is_object())
                {
                    // Apply changes
                    if (addressField != nullptr)
                    {
                        auto callback =
                            [asyncResp, entryIdx,
                             addressField{std::string(*addressField)}](
                                const boost::system::error_code ec) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                asyncResp->res
                                    .jsonValue["IPv4Addresses"][std::to_string(
                                        entryIdx)]["Address"] = addressField;
                            };

                        crow::connections::systemBus->async_method_call(
                            std::move(callback), "xyz.openbmc_project.Network",
                            "/xyz/openbmc_project/network/" + ifaceId +
                                "/ipv4/" + thisData->id,
                            "org.freedesktop.DBus.Properties", "Set",
                            "xyz.openbmc_project.Network.IP", "Address",
                            std::variant<std::string>(*addressField));
                    }

                    if (prefixLength && subnetField != nullptr)
                    {
                        changeIPv4SubnetMaskProperty(ifaceId, entryIdx,
                                                     thisData->id, *subnetField,
                                                     *prefixLength, asyncResp);
                    }

                    if (!addressOriginInDBusFormat.empty() &&
                        addressOriginField != nullptr)
                    {
                        changeIPv4Origin(ifaceId, entryIdx, thisData->id,
                                         *addressOriginField,
                                         addressOriginInDBusFormat, asyncResp);
                    }

                    if (gatewayField != nullptr)
                    {
                        auto callback =
                            [asyncResp, entryIdx,
                             gatewayField{std::string(*gatewayField)}](
                                const boost::system::error_code ec) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                asyncResp->res
                                    .jsonValue["IPv4Addresses"][std::to_string(
                                        entryIdx)]["Gateway"] =
                                    std::move(gatewayField);
                            };

                        crow::connections::systemBus->async_method_call(
                            std::move(callback), "xyz.openbmc_project.Network",
                            "/xyz/openbmc_project/network/" + ifaceId +
                                "/ipv4/" + thisData->id,
                            "org.freedesktop.DBus.Properties", "Set",
                            "xyz.openbmc_project.Network.IP", "Gateway",
                            std::variant<std::string>(*gatewayField));
                    }
                }
                thisData++;
            }
            else
            {
                // Create IPv4 with provided data
                if (gatewayField == nullptr)
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/Gateway");
                    continue;
                }

                if (addressField == nullptr)
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/Address");
                    continue;
                }

                if (!prefixLength)
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/SubnetMask");
                    continue;
                }

                createIPv4(ifaceId, entryIdx, *prefixLength, *gatewayField,
                           *addressField, asyncResp);
                asyncResp->res.jsonValue["IPv4Addresses"][entryIdx] = thisJson;
            }
            entryIdx++;
        }
    }

    void parseInterfaceData(
        nlohmann::json &json_response, const std::string &iface_id,
        const EthernetInterfaceData &ethData,
        const boost::container::flat_set<IPv4AddressData> &ipv4Data)
    {
        json_response["Id"] = iface_id;
        json_response["@odata.id"] =
            "/redfish/v1/Managers/bmc/EthernetInterfaces/" + iface_id;
        json_response["InterfaceEnabled"] = true;
        if (ethData.speed == 0)
        {
            json_response["LinkStatus"] = "NoLink";
            json_response["Status"] = {
                {"Health", "OK"},
                {"State", "Disabled"},
            };
        }
        else
        {
            json_response["LinkStatus"] = "LinkUp";
            json_response["Status"] = {
                {"Health", "OK"},
                {"State", "Enabled"},
            };
        }
        json_response["SpeedMbps"] = ethData.speed;
        json_response["MACAddress"] = ethData.mac_address;
        if (!ethData.hostname.empty())
        {
            json_response["HostName"] = ethData.hostname;
        }

        nlohmann::json &vlanObj = json_response["VLAN"];
        if (ethData.vlan_id)
        {
            vlanObj["VLANEnable"] = true;
            vlanObj["VLANId"] = *ethData.vlan_id;
        }
        else
        {
            vlanObj["VLANEnable"] = false;
            vlanObj["VLANId"] = 0;
        }
        json_response["NameServers"] = ethData.nameservers;

        if (ipv4Data.size() > 0)
        {
            nlohmann::json &ipv4_array = json_response["IPv4Addresses"];
            ipv4_array = nlohmann::json::array();
            for (auto &ipv4_config : ipv4Data)
            {
                ipv4_array.push_back({{"AddressOrigin", ipv4_config.origin},
                                      {"SubnetMask", ipv4_config.netmask},
                                      {"Address", ipv4_config.address},
                                      {"Gateway", ipv4_config.gateway}});
            }
        }
    }

    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        getEthernetIfaceData(
            params[0],
            [this, asyncResp, iface_id{std::string(params[0])}](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (!success)
                {
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(asyncResp->res,
                                               "EthernetInterface", iface_id);
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#EthernetInterface.v1_2_0.EthernetInterface";
                asyncResp->res.jsonValue["@odata.context"] =
                    "/redfish/v1/$metadata#EthernetInterface.EthernetInterface";
                asyncResp->res.jsonValue["Name"] = "Manager Ethernet Interface";
                asyncResp->res.jsonValue["Description"] =
                    "Management Network Interface";

                parseInterfaceData(asyncResp->res.jsonValue, iface_id, ethData,
                                   ipv4Data);
            });
    }

    void doPatch(crow::Response &res, const crow::Request &req,
                 const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string &iface_id = params[0];

        std::optional<nlohmann::json> vlan;
        std::optional<std::string> hostname;
        std::optional<nlohmann::json> ipv4Addresses;
        std::optional<nlohmann::json> ipv6Addresses;

        if (!json_util::readJson(req, res, "VLAN", vlan, "HostName", hostname,
                                 "IPv4Addresses", ipv4Addresses,
                                 "IPv6Addresses", ipv6Addresses))
        {
            return;
        }
        std::optional<uint64_t> vlanId = 0;
        std::optional<bool> vlanEnable = false;
        if (vlan)
        {
            if (!json_util::readJson(*vlan, res, "VLANEnable", vlanEnable,
                                     "VLANId", vlanId))
            {
                return;
            }
            // Need both vlanId and vlanEnable to service this request
            if (static_cast<bool>(vlanId) ^ static_cast<bool>(vlanEnable))
            {
                if (vlanId)
                {
                    messages::propertyMissing(asyncResp->res, "VLANEnable");
                }
                else
                {
                    messages::propertyMissing(asyncResp->res, "VLANId");
                }

                return;
            }
        }

        // Get single eth interface data, and call the below callback for JSON
        // preparation
        getEthernetIfaceData(
            iface_id,
            [this, asyncResp, iface_id, vlanId, vlanEnable,
             hostname = std::move(hostname),
             ipv4Addresses = std::move(ipv4Addresses),
             ipv6Addresses = std::move(ipv6Addresses)](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (!success)
                {
                    // ... otherwise return error
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(
                        asyncResp->res, "VLAN Network Interface", iface_id);
                    return;
                }

                parseInterfaceData(asyncResp->res.jsonValue, iface_id, ethData,
                                   ipv4Data);

                if (vlanId && vlanEnable)
                {
                    handleVlanPatch(iface_id, *vlanId, *vlanEnable, ethData,
                                    asyncResp);
                }

                if (hostname)
                {
                    handleHostnamePatch(*hostname, asyncResp);
                }

                if (ipv4Addresses)
                {
                    handleIPv4Patch(iface_id, *ipv4Addresses, ipv4Data,
                                    asyncResp);
                }

                if (ipv6Addresses)
                {
                    // TODO(kkowalsk) IPv6 Not supported on D-Bus yet
                    messages::propertyNotWritable(asyncResp->res,
                                                  "IPv6Addresses");
                }
            });
    }
};

/**
 * VlanNetworkInterface derived class for delivering VLANNetworkInterface
 * Schema
 */
class VlanNetworkInterface : public Node
{
  public:
    /*
     * Default Constructor
     */
    template <typename CrowApp>
    VlanNetworkInterface(CrowApp &app) :
        Node(app,
             "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/<str>",
             std::string(), std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void parseInterfaceData(
        nlohmann::json &json_response, const std::string &parent_iface_id,
        const std::string &iface_id, const EthernetInterfaceData &ethData,
        const boost::container::flat_set<IPv4AddressData> &ipv4Data)
    {
        // Fill out obvious data...
        json_response["Id"] = iface_id;
        json_response["@odata.id"] =
            "/redfish/v1/Managers/bmc/EthernetInterfaces/" + parent_iface_id +
            "/VLANs/" + iface_id;

        json_response["VLANEnable"] = true;
        if (ethData.vlan_id)
        {
            json_response["VLANId"] = *ethData.vlan_id;
        }
    }

    bool verifyNames(crow::Response &res, const std::string &parent,
                     const std::string &iface)
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (!boost::starts_with(iface, parent + "_"))
        {
            messages::resourceNotFound(asyncResp->res, "VLAN Network Interface",
                                       iface);
            return false;
        }
        else
        {
            return true;
        }
    }

    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // TODO(Pawel) this shall be parameterized call (two params) to get
        // EthernetInterfaces for any Manager, not only hardcoded 'openbmc'.
        // Check if there is required param, truly entering this shall be
        // impossible.
        if (params.size() != 2)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        const std::string &parent_iface_id = params[0];
        const std::string &iface_id = params[1];
        res.jsonValue["@odata.type"] =
            "#VLanNetworkInterface.v1_1_0.VLanNetworkInterface";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#VLanNetworkInterface.VLanNetworkInterface";
        res.jsonValue["Name"] = "VLAN Network Interface";

        if (!verifyNames(res, parent_iface_id, iface_id))
        {
            return;
        }

        // Get single eth interface data, and call the below callback for JSON
        // preparation
        getEthernetIfaceData(
            iface_id,
            [this, asyncResp, parent_iface_id, iface_id](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (success && ethData.vlan_id)
                {
                    parseInterfaceData(asyncResp->res.jsonValue,
                                       parent_iface_id, iface_id, ethData,
                                       ipv4Data);
                }
                else
                {
                    // ... otherwise return error
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(
                        asyncResp->res, "VLAN Network Interface", iface_id);
                }
            });
    }

    void doPatch(crow::Response &res, const crow::Request &req,
                 const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string &parentIfaceId = params[0];
        const std::string &ifaceId = params[1];

        if (!verifyNames(res, parentIfaceId, ifaceId))
        {
            return;
        }

        bool vlanEnable = false;
        uint64_t vlanId = 0;

        if (!json_util::readJson(req, res, "VLANEnable", vlanEnable, "VLANId",
                                 vlanId))
        {
            return;
        }

        // Get single eth interface data, and call the below callback for JSON
        // preparation
        getEthernetIfaceData(
            ifaceId,
            [this, asyncResp, parentIfaceId, ifaceId, vlanEnable, vlanId](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (!success)
                {
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(
                        asyncResp->res, "VLAN Network Interface", ifaceId);

                    return;
                }

                parseInterfaceData(asyncResp->res.jsonValue, parentIfaceId,
                                   ifaceId, ethData, ipv4Data);

                EthernetInterface::handleVlanPatch(ifaceId, vlanId, vlanEnable,
                                                   ethData, asyncResp);
            });
    }

    void doDelete(crow::Response &res, const crow::Request &req,
                  const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string &parentIfaceId = params[0];
        const std::string &ifaceId = params[1];

        if (!verifyNames(asyncResp->res, parentIfaceId, ifaceId))
        {
            return;
        }

        // Get single eth interface data, and call the below callback for JSON
        // preparation
        getEthernetIfaceData(
            ifaceId,
            [this, asyncResp, parentIfaceId{std::string(parentIfaceId)},
             ifaceId{std::string(ifaceId)}](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (success && ethData.vlan_id)
                {
                    parseInterfaceData(asyncResp->res.jsonValue, parentIfaceId,
                                       ifaceId, ethData, ipv4Data);

                    auto callback =
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                messages::internalError(asyncResp->res);
                            }
                        };
                    crow::connections::systemBus->async_method_call(
                        std::move(callback), "xyz.openbmc_project.Network",
                        std::string("/xyz/openbmc_project/network/") + ifaceId,
                        "xyz.openbmc_project.Object.Delete", "Delete");
                }
                else
                {
                    // ... otherwise return error
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(
                        asyncResp->res, "VLAN Network Interface", ifaceId);
                }
            });
    }
};

/**
 * VlanNetworkInterfaceCollection derived class for delivering
 * VLANNetworkInterface Collection Schema
 */
class VlanNetworkInterfaceCollection : public Node
{
  public:
    template <typename CrowApp>
    VlanNetworkInterfaceCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/",
             std::string())
    {
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            // This means there is a problem with the router
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string &rootInterfaceName = params[0];

        // Get eth interface list, and call the below callback for JSON
        // preparation
        getEthernetIfaceList(
            [this, asyncResp,
             rootInterfaceName{std::string(rootInterfaceName)}](
                const bool &success,
                const std::vector<std::string> &iface_list) {
                if (!success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#VLanNetworkInterfaceCollection."
                    "VLanNetworkInterfaceCollection";
                asyncResp->res.jsonValue["@odata.context"] =
                    "/redfish/v1/$metadata"
                    "#VLanNetworkInterfaceCollection."
                    "VLanNetworkInterfaceCollection";
                asyncResp->res.jsonValue["Name"] =
                    "VLAN Network Interface Collection";

                nlohmann::json iface_array = nlohmann::json::array();

                for (const std::string &iface_item : iface_list)
                {
                    if (boost::starts_with(iface_item, rootInterfaceName + "_"))
                    {
                        iface_array.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                                  rootInterfaceName + "/VLANs/" + iface_item}});
                    }
                }

                if (iface_array.empty())
                {
                    messages::resourceNotFound(
                        asyncResp->res, "EthernetInterface", rootInterfaceName);
                    return;
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    iface_array.size();
                asyncResp->res.jsonValue["Members"] = std::move(iface_array);
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                    rootInterfaceName + "/VLANs";
            });
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        uint32_t vlanId = 0;
        if (!json_util::readJson(req, res, "VLANId", vlanId))
        {
            return;
        }
        const std::string &rootInterfaceName = params[0];
        auto callback = [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                // TODO(ed) make more consistent error messages based on
                // phosphor-network responses
                messages::internalError(asyncResp->res);
                return;
            }
            messages::created(asyncResp->res);
        };
        crow::connections::systemBus->async_method_call(
            std::move(callback), "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network",
            "xyz.openbmc_project.Network.VLAN.Create", "VLAN",
            rootInterfaceName, vlanId);
    }
};
} // namespace redfish
