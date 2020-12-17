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
#include <utils/json_utils.hpp>

#include <optional>
#include <regex>
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
            std::string,
            std::variant<std::string, bool, uint8_t, int16_t, uint16_t, int32_t,
                         uint32_t, int64_t, uint64_t, double,
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
    bool isActive;

    bool operator<(const IPv4AddressData& obj) const
    {
        return id < obj.id;
    }
};

/**
 * Structure for keeping IPv6 data required by Redfish
 */
struct IPv6AddressData
{
    std::string id;
    std::string address;
    std::string origin;
    uint8_t prefixLength;

    bool operator<(const IPv6AddressData& obj) const
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
    bool DNSEnabled;
    bool NTPEnabled;
    bool HostNameEnabled;
    bool SendHostNameEnabled;
    bool linkUp;
    bool nicEnabled;
    std::string DHCPEnabled;
    std::string operatingMode;
    std::string hostname;
    std::string default_gateway;
    std::string ipv6_default_gateway;
    std::string mac_address;
    std::vector<std::uint32_t> vlan_id;
    std::vector<std::string> nameServers;
    std::vector<std::string> staticNameServers;
    std::vector<std::string> domainnames;
};

struct DHCPParameters
{
    std::optional<bool> dhcpv4Enabled;
    std::optional<bool> useDNSServers;
    std::optional<bool> useNTPServers;
    std::optional<bool> useUseDomainName;
    std::optional<std::string> dhcpv6OperatingMode;
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

inline bool translateDHCPEnabledToBool(const std::string& inputDHCP,
                                       bool isIPv4)
{
    if (isIPv4)
    {
        return (
            (inputDHCP ==
             "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.v4") ||
            (inputDHCP ==
             "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.both"));
    }
    return ((inputDHCP ==
             "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.v6") ||
            (inputDHCP ==
             "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.both"));
}

inline std::string getDhcpEnabledEnumeration(bool isIPv4, bool isIPv6)
{
    if (isIPv4 && isIPv6)
    {
        return "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.both";
    }
    if (isIPv4)
    {
        return "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.v4";
    }
    if (isIPv6)
    {
        return "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.v6";
    }
    return "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.none";
}

inline std::string
    translateAddressOriginDbusToRedfish(const std::string& inputOrigin,
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
        return "LinkLocal";
    }
    if (inputOrigin == "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP")
    {
        if (isIPv4)
        {
            return "DHCP";
        }
        return "DHCPv6";
    }
    if (inputOrigin == "xyz.openbmc_project.Network.IP.AddressOrigin.SLAAC")
    {
        return "SLAAC";
    }
    return "";
}

inline bool extractEthernetInterfaceData(const std::string& ethifaceId,
                                         GetManagedObjects& dbusData,
                                         EthernetInterfaceData& ethData)
{
    bool idFound = false;
    for (auto& objpath : dbusData)
    {
        for (auto& ifacePair : objpath.second)
        {
            if (objpath.first == "/xyz/openbmc_project/network/" + ethifaceId)
            {
                idFound = true;
                if (ifacePair.first == "xyz.openbmc_project.Network.MACAddress")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "MACAddress")
                        {
                            const std::string* mac =
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
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "Id")
                        {
                            const uint32_t* id =
                                std::get_if<uint32_t>(&propertyPair.second);
                            if (id != nullptr)
                            {
                                ethData.vlan_id.push_back(*id);
                            }
                        }
                    }
                }
                else if (ifacePair.first ==
                         "xyz.openbmc_project.Network.EthernetInterface")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "AutoNeg")
                        {
                            const bool* autoNeg =
                                std::get_if<bool>(&propertyPair.second);
                            if (autoNeg != nullptr)
                            {
                                ethData.auto_neg = *autoNeg;
                            }
                        }
                        else if (propertyPair.first == "Speed")
                        {
                            const uint32_t* speed =
                                std::get_if<uint32_t>(&propertyPair.second);
                            if (speed != nullptr)
                            {
                                ethData.speed = *speed;
                            }
                        }
                        else if (propertyPair.first == "LinkUp")
                        {
                            const bool* linkUp =
                                std::get_if<bool>(&propertyPair.second);
                            if (linkUp != nullptr)
                            {
                                ethData.linkUp = *linkUp;
                            }
                        }
                        else if (propertyPair.first == "NICEnabled")
                        {
                            const bool* nicEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (nicEnabled != nullptr)
                            {
                                ethData.nicEnabled = *nicEnabled;
                            }
                        }
                        else if (propertyPair.first == "Nameservers")
                        {
                            const std::vector<std::string>* nameservers =
                                std::get_if<std::vector<std::string>>(
                                    &propertyPair.second);
                            if (nameservers != nullptr)
                            {
                                ethData.nameServers = *nameservers;
                            }
                        }
                        else if (propertyPair.first == "StaticNameServers")
                        {
                            const std::vector<std::string>* staticNameServers =
                                std::get_if<std::vector<std::string>>(
                                    &propertyPair.second);
                            if (staticNameServers != nullptr)
                            {
                                ethData.staticNameServers = *staticNameServers;
                            }
                        }
                        else if (propertyPair.first == "DHCPEnabled")
                        {
                            const std::string* dhcpEnabled =
                                std::get_if<std::string>(&propertyPair.second);
                            if (dhcpEnabled != nullptr)
                            {
                                ethData.DHCPEnabled = *dhcpEnabled;
                            }
                        }
                        else if (propertyPair.first == "DomainName")
                        {
                            const std::vector<std::string>* domainNames =
                                std::get_if<std::vector<std::string>>(
                                    &propertyPair.second);
                            if (domainNames != nullptr)
                            {
                                ethData.domainnames = *domainNames;
                            }
                        }
                    }
                }
            }

            if (objpath.first == "/xyz/openbmc_project/network/config/dhcp")
            {
                if (ifacePair.first ==
                    "xyz.openbmc_project.Network.DHCPConfiguration")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "DNSEnabled")
                        {
                            const bool* dnsEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (dnsEnabled != nullptr)
                            {
                                ethData.DNSEnabled = *dnsEnabled;
                            }
                        }
                        else if (propertyPair.first == "NTPEnabled")
                        {
                            const bool* ntpEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (ntpEnabled != nullptr)
                            {
                                ethData.NTPEnabled = *ntpEnabled;
                            }
                        }
                        else if (propertyPair.first == "HostNameEnabled")
                        {
                            const bool* hostNameEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (hostNameEnabled != nullptr)
                            {
                                ethData.HostNameEnabled = *hostNameEnabled;
                            }
                        }
                        else if (propertyPair.first == "SendHostNameEnabled")
                        {
                            const bool* sendHostNameEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (sendHostNameEnabled != nullptr)
                            {
                                ethData.SendHostNameEnabled =
                                    *sendHostNameEnabled;
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
                for (const auto& propertyPair : ifacePair.second)
                {
                    if (propertyPair.first == "HostName")
                    {
                        const std::string* hostname =
                            std::get_if<std::string>(&propertyPair.second);
                        if (hostname != nullptr)
                        {
                            ethData.hostname = *hostname;
                        }
                    }
                    else if (propertyPair.first == "DefaultGateway")
                    {
                        const std::string* defaultGateway =
                            std::get_if<std::string>(&propertyPair.second);
                        if (defaultGateway != nullptr)
                        {
                            ethData.default_gateway = *defaultGateway;
                        }
                    }
                    else if (propertyPair.first == "DefaultGateway6")
                    {
                        const std::string* defaultGateway6 =
                            std::get_if<std::string>(&propertyPair.second);
                        if (defaultGateway6 != nullptr)
                        {
                            ethData.ipv6_default_gateway = *defaultGateway6;
                        }
                    }
                }
            }
        }
    }
    return idFound;
}

// Helper function that extracts data for single ethernet ipv6 address
inline void
    extractIPV6Data(const std::string& ethifaceId,
                    const GetManagedObjects& dbusData,
                    boost::container::flat_set<IPv6AddressData>& ipv6Config)
{
    const std::string ipv6PathStart =
        "/xyz/openbmc_project/network/" + ethifaceId + "/ipv6/";

    // Since there might be several IPv6 configurations aligned with
    // single ethernet interface, loop over all of them
    for (const auto& objpath : dbusData)
    {
        // Check if proper pattern for object path appears
        if (boost::starts_with(objpath.first.str, ipv6PathStart))
        {
            for (auto& interface : objpath.second)
            {
                if (interface.first == "xyz.openbmc_project.Network.IP")
                {
                    // Instance IPv6AddressData structure, and set as
                    // appropriate
                    std::pair<
                        boost::container::flat_set<IPv6AddressData>::iterator,
                        bool>
                        it = ipv6Config.insert(IPv6AddressData{});
                    IPv6AddressData& ipv6Address = *it.first;
                    ipv6Address.id =
                        objpath.first.str.substr(ipv6PathStart.size());
                    for (auto& property : interface.second)
                    {
                        if (property.first == "Address")
                        {
                            const std::string* address =
                                std::get_if<std::string>(&property.second);
                            if (address != nullptr)
                            {
                                ipv6Address.address = *address;
                            }
                        }
                        else if (property.first == "Origin")
                        {
                            const std::string* origin =
                                std::get_if<std::string>(&property.second);
                            if (origin != nullptr)
                            {
                                ipv6Address.origin =
                                    translateAddressOriginDbusToRedfish(*origin,
                                                                        false);
                            }
                        }
                        else if (property.first == "PrefixLength")
                        {
                            const uint8_t* prefix =
                                std::get_if<uint8_t>(&property.second);
                            if (prefix != nullptr)
                            {
                                ipv6Address.prefixLength = *prefix;
                            }
                        }
                        else
                        {
                            BMCWEB_LOG_ERROR
                                << "Got extra property: " << property.first
                                << " on the " << objpath.first.str << " object";
                        }
                    }
                }
            }
        }
    }
}

// Helper function that extracts data for single ethernet ipv4 address
inline void
    extractIPData(const std::string& ethifaceId,
                  const GetManagedObjects& dbusData,
                  boost::container::flat_set<IPv4AddressData>& ipv4Config)
{
    const std::string ipv4PathStart =
        "/xyz/openbmc_project/network/" + ethifaceId + "/ipv4/";

    // Since there might be several IPv4 configurations aligned with
    // single ethernet interface, loop over all of them
    for (const auto& objpath : dbusData)
    {
        // Check if proper pattern for object path appears
        if (boost::starts_with(objpath.first.str, ipv4PathStart))
        {
            for (auto& interface : objpath.second)
            {
                if (interface.first == "xyz.openbmc_project.Network.IP")
                {
                    // Instance IPv4AddressData structure, and set as
                    // appropriate
                    std::pair<
                        boost::container::flat_set<IPv4AddressData>::iterator,
                        bool>
                        it = ipv4Config.insert(IPv4AddressData{});
                    IPv4AddressData& ipv4Address = *it.first;
                    ipv4Address.id =
                        objpath.first.str.substr(ipv4PathStart.size());
                    for (auto& property : interface.second)
                    {
                        if (property.first == "Address")
                        {
                            const std::string* address =
                                std::get_if<std::string>(&property.second);
                            if (address != nullptr)
                            {
                                ipv4Address.address = *address;
                            }
                        }
                        else if (property.first == "Gateway")
                        {
                            const std::string* gateway =
                                std::get_if<std::string>(&property.second);
                            if (gateway != nullptr)
                            {
                                ipv4Address.gateway = *gateway;
                            }
                        }
                        else if (property.first == "Origin")
                        {
                            const std::string* origin =
                                std::get_if<std::string>(&property.second);
                            if (origin != nullptr)
                            {
                                ipv4Address.origin =
                                    translateAddressOriginDbusToRedfish(*origin,
                                                                        true);
                            }
                        }
                        else if (property.first == "PrefixLength")
                        {
                            const uint8_t* mask =
                                std::get_if<uint8_t>(&property.second);
                            if (mask != nullptr)
                            {
                                // convert it to the string
                                ipv4Address.netmask = getNetmask(*mask);
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
                    ipv4Address.linktype =
                        boost::starts_with(ipv4Address.address, "169.254.")
                            ? LinkType::Local
                            : LinkType::Global;
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
void changeVlanId(const std::string& ifaceId, const uint32_t& inputVlanId,
                  CallbackFunc&& callback)
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
inline bool ipv4VerifyIpAndGetBitcount(const std::string& ip,
                                       uint8_t* bits = nullptr)
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

    char* endPtr;
    long previousValue = 255;
    bool firstZeroInByteHit;
    for (const std::string& byte : bytesInMask)
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
            for (long bitIdx = 7; bitIdx >= 0; bitIdx--)
            {
                if (value & (1L << bitIdx))
                {
                    if (firstZeroInByteHit)
                    {
                        // Continuity not preserved
                        return false;
                    }
                    (*bits)++;
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
 * @brief Deletes given IPv4 interface
 *
 * @param[in] ifaceId     Id of interface whose IP should be deleted
 * @param[in] ipHash      DBus Hash id of IP that should be deleted
 * @param[io] asyncResp   Response object that will be returned to client
 *
 * @return None
 */
inline void deleteIPv4(const std::string& ifaceId, const std::string& ipHash,
                       const std::shared_ptr<AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
        },
        "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + ipHash,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

/**
 * @brief Creates a static IPv4 entry
 *
 * @param[in] ifaceId      Id of interface upon which to create the IPv4 entry
 * @param[in] prefixLength IPv4 prefix syntax for the subnet mask
 * @param[in] gateway      IPv4 address of this interfaces gateway
 * @param[in] address      IPv4 address to assign to this interface
 * @param[io] asyncResp    Response object that will be returned to client
 *
 * @return None
 */
inline void createIPv4(const std::string& ifaceId, uint8_t prefixLength,
                       const std::string& gateway, const std::string& address,
                       const std::shared_ptr<AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
        },
        "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId,
        "xyz.openbmc_project.Network.IP.Create", "IP",
        "xyz.openbmc_project.Network.IP.Protocol.IPv4", address, prefixLength,
        gateway);
}

/**
 * @brief Deletes the IPv4 entry for this interface and creates a replacement
 * static IPv4 entry
 *
 * @param[in] ifaceId      Id of interface upon which to create the IPv4 entry
 * @param[in] id           The unique hash entry identifying the DBus entry
 * @param[in] prefixLength IPv4 prefix syntax for the subnet mask
 * @param[in] gateway      IPv4 address of this interfaces gateway
 * @param[in] address      IPv4 address to assign to this interface
 * @param[io] asyncResp    Response object that will be returned to client
 *
 * @return None
 */
inline void deleteAndCreateIPv4(const std::string& ifaceId,
                                const std::string& id, uint8_t prefixLength,
                                const std::string& gateway,
                                const std::string& address,
                                const std::shared_ptr<AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, ifaceId, address, prefixLength,
         gateway](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec2) {
                    if (ec2)
                    {
                        messages::internalError(asyncResp->res);
                    }
                },
                "xyz.openbmc_project.Network",
                "/xyz/openbmc_project/network/" + ifaceId,
                "xyz.openbmc_project.Network.IP.Create", "IP",
                "xyz.openbmc_project.Network.IP.Protocol.IPv4", address,
                prefixLength, gateway);
        },
        "xyz.openbmc_project.Network",
        +"/xyz/openbmc_project/network/" + ifaceId + "/ipv4/" + id,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

/**
 * @brief Deletes given IPv6
 *
 * @param[in] ifaceId     Id of interface whose IP should be deleted
 * @param[in] ipHash      DBus Hash id of IP that should be deleted
 * @param[io] asyncResp   Response object that will be returned to client
 *
 * @return None
 */
inline void deleteIPv6(const std::string& ifaceId, const std::string& ipHash,
                       const std::shared_ptr<AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
        },
        "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId + "/ipv6/" + ipHash,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

/**
 * @brief Deletes the IPv6 entry for this interface and creates a replacement
 * static IPv6 entry
 *
 * @param[in] ifaceId      Id of interface upon which to create the IPv6 entry
 * @param[in] id           The unique hash entry identifying the DBus entry
 * @param[in] prefixLength IPv6 prefix syntax for the subnet mask
 * @param[in] address      IPv6 address to assign to this interface
 * @param[io] asyncResp    Response object that will be returned to client
 *
 * @return None
 */
inline void deleteAndCreateIPv6(const std::string& ifaceId,
                                const std::string& id, uint8_t prefixLength,
                                const std::string& address,
                                const std::shared_ptr<AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, ifaceId, address,
         prefixLength](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec2) {
                    if (ec2)
                    {
                        messages::internalError(asyncResp->res);
                    }
                },
                "xyz.openbmc_project.Network",
                "/xyz/openbmc_project/network/" + ifaceId,
                "xyz.openbmc_project.Network.IP.Create", "IP",
                "xyz.openbmc_project.Network.IP.Protocol.IPv6", address,
                prefixLength, "");
        },
        "xyz.openbmc_project.Network",
        +"/xyz/openbmc_project/network/" + ifaceId + "/ipv6/" + id,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

/**
 * @brief Creates IPv6 with given data
 *
 * @param[in] ifaceId      Id of interface whose IP should be added
 * @param[in] prefixLength Prefix length that needs to be added
 * @param[in] address      IP address that needs to be added
 * @param[io] asyncResp    Response object that will be returned to client
 *
 * @return None
 */
inline void createIPv6(const std::string& ifaceId, uint8_t prefixLength,
                       const std::string& address,
                       const std::shared_ptr<AsyncResp>& asyncResp)
{
    auto createIpHandler = [asyncResp](const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
        }
    };
    // Passing null for gateway, as per redfish spec IPv6StaticAddresses object
    // does not have associated gateway property
    crow::connections::systemBus->async_method_call(
        std::move(createIpHandler), "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId,
        "xyz.openbmc_project.Network.IP.Create", "IP",
        "xyz.openbmc_project.Network.IP.Protocol.IPv6", address, prefixLength,
        "");
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
void getEthernetIfaceData(const std::string& ethifaceId,
                          CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [ethifaceId{std::string{ethifaceId}}, callback{std::move(callback)}](
            const boost::system::error_code errorCode,
            GetManagedObjects& resp) {
            EthernetInterfaceData ethData{};
            boost::container::flat_set<IPv4AddressData> ipv4Data;
            boost::container::flat_set<IPv6AddressData> ipv6Data;

            if (errorCode)
            {
                callback(false, ethData, ipv4Data, ipv6Data);
                return;
            }

            bool found =
                extractEthernetInterfaceData(ethifaceId, resp, ethData);
            if (!found)
            {
                callback(false, ethData, ipv4Data, ipv6Data);
                return;
            }

            extractIPData(ethifaceId, resp, ipv4Data);
            // Fix global GW
            for (IPv4AddressData& ipv4 : ipv4Data)
            {
                if (((ipv4.linktype == LinkType::Global) &&
                     (ipv4.gateway == "0.0.0.0")) ||
                    (ipv4.origin == "DHCP"))
                {
                    ipv4.gateway = ethData.default_gateway;
                }
            }

            extractIPV6Data(ethifaceId, resp, ipv6Data);
            // Finally make a callback with useful data
            callback(true, ethData, ipv4Data, ipv6Data);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
 * Function that retrieves all Ethernet Interfaces available through Network
 * Manager
 * @param callback a function that shall be called to convert Dbus output
 * into JSON.
 */
template <typename CallbackFunc>
void getEthernetIfaceList(CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code errorCode,
            GetManagedObjects& resp) {
            // Callback requires vector<string> to retrieve all available
            // ethernet interfaces
            boost::container::flat_set<std::string> ifaceList;
            ifaceList.reserve(resp.size());
            if (errorCode)
            {
                callback(false, ifaceList);
                return;
            }

            // Iterate over all retrieved ObjectPaths.
            for (const auto& objpath : resp)
            {
                // And all interfaces available for certain ObjectPath.
                for (const auto& interface : objpath.second)
                {
                    // If interface is
                    // xyz.openbmc_project.Network.EthernetInterface, this is
                    // what we're looking for.
                    if (interface.first ==
                        "xyz.openbmc_project.Network.EthernetInterface")
                    {
                        // Cut out everything until last "/", ...
                        const std::string& ifaceId = objpath.first.str;
                        std::size_t lastPos = ifaceId.rfind('/');
                        if (lastPos != std::string::npos)
                        {
                            // and put it into output vector.
                            ifaceList.emplace(ifaceId.substr(lastPos + 1));
                        }
                    }
                }
            }
            // Finally make a callback with useful data
            callback(true, ifaceList);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
 * EthernetCollection derived class for delivering Ethernet Collection Schema
 */
class EthernetCollection : public Node
{
  public:
    EthernetCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] =
            "#EthernetInterfaceCollection.EthernetInterfaceCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/EthernetInterfaces";
        res.jsonValue["Name"] = "Ethernet Network Interface Collection";
        res.jsonValue["Description"] =
            "Collection of EthernetInterfaces for this Manager";
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Get eth interface list, and call the below callback for JSON
        // preparation
        getEthernetIfaceList(
            [asyncResp](
                const bool& success,
                const boost::container::flat_set<std::string>& ifaceList) {
                if (!success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                nlohmann::json& ifaceArray =
                    asyncResp->res.jsonValue["Members"];
                ifaceArray = nlohmann::json::array();
                std::string tag = "_";
                for (const std::string& ifaceItem : ifaceList)
                {
                    std::size_t found = ifaceItem.find(tag);
                    if (found == std::string::npos)
                    {
                        ifaceArray.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                                  ifaceItem}});
                    }
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    ifaceArray.size();
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/EthernetInterfaces";
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
    EthernetInterface(App& app) :
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

  private:
    void handleHostnamePatch(const std::string& hostname,
                             const std::shared_ptr<AsyncResp>& asyncResp)
    {
        // SHOULD handle host names of up to 255 characters(RFC 1123)
        if (hostname.length() > 255)
        {
            messages::propertyValueFormatError(asyncResp->res, hostname,
                                               "HostName");
            return;
        }
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

    void handleDomainnamePatch(const std::string& ifaceId,
                               const std::string& domainname,
                               const std::shared_ptr<AsyncResp>& asyncResp)
    {
        std::vector<std::string> vectorDomainname = {domainname};
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/" + ifaceId,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.EthernetInterface", "DomainName",
            std::variant<std::vector<std::string>>(vectorDomainname));
    }

    void handleFqdnPatch(const std::string& ifaceId, const std::string& fqdn,
                         const std::shared_ptr<AsyncResp>& asyncResp)
    {
        // Total length of FQDN must not exceed 255 characters(RFC 1035)
        if (fqdn.length() > 255)
        {
            messages::propertyValueFormatError(asyncResp->res, fqdn, "FQDN");
            return;
        }

        size_t pos = fqdn.find('.');
        if (pos == std::string::npos)
        {
            messages::propertyValueFormatError(asyncResp->res, fqdn, "FQDN");
            return;
        }

        std::string hostname;
        std::string domainname;
        domainname = (fqdn).substr(pos + 1);
        hostname = (fqdn).substr(0, pos);

        if (!isHostnameValid(hostname) || !isDomainnameValid(domainname))
        {
            messages::propertyValueFormatError(asyncResp->res, fqdn, "FQDN");
            return;
        }

        handleHostnamePatch(hostname, asyncResp);
        handleDomainnamePatch(ifaceId, domainname, asyncResp);
    }

    bool isHostnameValid(const std::string& hostname)
    {
        // A valid host name can never have the dotted-decimal form (RFC 1123)
        if (std::all_of(hostname.begin(), hostname.end(), ::isdigit))
        {
            return false;
        }
        // Each label(hostname/subdomains) within a valid FQDN
        // MUST handle host names of up to 63 characters (RFC 1123)
        // labels cannot start or end with hyphens (RFC 952)
        // labels can start with numbers (RFC 1123)
        const std::regex pattern(
            "^[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]$");

        return std::regex_match(hostname, pattern);
    }

    bool isDomainnameValid(const std::string& domainname)
    {
        // Can have multiple subdomains
        // Top Level Domain's min length is 2 character
        const std::regex pattern("^([A-Za-z0-9][a-zA-Z0-9\\-]{1,61}|[a-zA-Z0-9]"
                                 "{1,30}\\.)*[a-zA-Z]{2,}$");

        return std::regex_match(domainname, pattern);
    }

    void handleMACAddressPatch(const std::string& ifaceId,
                               const std::string& macAddress,
                               const std::shared_ptr<AsyncResp>& asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp, macAddress](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/" + ifaceId,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.MACAddress", "MACAddress",
            std::variant<std::string>(macAddress));
    }

    void setDHCPEnabled(const std::string& ifaceId,
                        const std::string& propertyName, const bool v4Value,
                        const bool v6Value,
                        const std::shared_ptr<AsyncResp>& asyncResp)
    {
        const std::string dhcp = getDhcpEnabledEnumeration(v4Value, v6Value);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/" + ifaceId,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.EthernetInterface", propertyName,
            std::variant<std::string>{dhcp});
    }

    void setEthernetInterfaceBoolProperty(
        const std::string& ifaceId, const std::string& propertyName,
        const bool& value, const std::shared_ptr<AsyncResp>& asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/" + ifaceId,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.EthernetInterface", propertyName,
            std::variant<bool>{value});
    }

    void setDHCPv4Config(const std::string& propertyName, const bool& value,
                         const std::shared_ptr<AsyncResp>& asyncResp)
    {
        BMCWEB_LOG_DEBUG << propertyName << " = " << value;
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/config/dhcp",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.DHCPConfiguration", propertyName,
            std::variant<bool>{value});
    }

    void handleDHCPPatch(const std::string& ifaceId,
                         const EthernetInterfaceData& ethData,
                         const DHCPParameters& v4dhcpParms,
                         const DHCPParameters& v6dhcpParms,
                         const std::shared_ptr<AsyncResp>& asyncResp)
    {
        bool ipv4Active = translateDHCPEnabledToBool(ethData.DHCPEnabled, true);
        bool ipv6Active =
            translateDHCPEnabledToBool(ethData.DHCPEnabled, false);

        bool nextv4DHCPState =
            v4dhcpParms.dhcpv4Enabled ? *v4dhcpParms.dhcpv4Enabled : ipv4Active;

        bool nextv6DHCPState{};
        if (v6dhcpParms.dhcpv6OperatingMode)
        {
            if ((*v6dhcpParms.dhcpv6OperatingMode != "Stateful") &&
                (*v6dhcpParms.dhcpv6OperatingMode != "Stateless") &&
                (*v6dhcpParms.dhcpv6OperatingMode != "Disabled"))
            {
                messages::propertyValueFormatError(
                    asyncResp->res, *v6dhcpParms.dhcpv6OperatingMode,
                    "OperatingMode");
                return;
            }
            nextv6DHCPState = (*v6dhcpParms.dhcpv6OperatingMode == "Stateful");
        }
        else
        {
            nextv6DHCPState = ipv6Active;
        }

        bool nextDNS{};
        if (v4dhcpParms.useDNSServers && v6dhcpParms.useDNSServers)
        {
            if (*v4dhcpParms.useDNSServers != *v6dhcpParms.useDNSServers)
            {
                messages::generalError(asyncResp->res);
                return;
            }
            nextDNS = *v4dhcpParms.useDNSServers;
        }
        else if (v4dhcpParms.useDNSServers)
        {
            nextDNS = *v4dhcpParms.useDNSServers;
        }
        else if (v6dhcpParms.useDNSServers)
        {
            nextDNS = *v6dhcpParms.useDNSServers;
        }
        else
        {
            nextDNS = ethData.DNSEnabled;
        }

        bool nextNTP{};
        if (v4dhcpParms.useNTPServers && v6dhcpParms.useNTPServers)
        {
            if (*v4dhcpParms.useNTPServers != *v6dhcpParms.useNTPServers)
            {
                messages::generalError(asyncResp->res);
                return;
            }
            nextNTP = *v4dhcpParms.useNTPServers;
        }
        else if (v4dhcpParms.useNTPServers)
        {
            nextNTP = *v4dhcpParms.useNTPServers;
        }
        else if (v6dhcpParms.useNTPServers)
        {
            nextNTP = *v6dhcpParms.useNTPServers;
        }
        else
        {
            nextNTP = ethData.NTPEnabled;
        }

        bool nextUseDomain{};
        if (v4dhcpParms.useUseDomainName && v6dhcpParms.useUseDomainName)
        {
            if (*v4dhcpParms.useUseDomainName != *v6dhcpParms.useUseDomainName)
            {
                messages::generalError(asyncResp->res);
                return;
            }
            nextUseDomain = *v4dhcpParms.useUseDomainName;
        }
        else if (v4dhcpParms.useUseDomainName)
        {
            nextUseDomain = *v4dhcpParms.useUseDomainName;
        }
        else if (v6dhcpParms.useUseDomainName)
        {
            nextUseDomain = *v6dhcpParms.useUseDomainName;
        }
        else
        {
            nextUseDomain = ethData.HostNameEnabled;
        }

        BMCWEB_LOG_DEBUG << "set DHCPEnabled...";
        setDHCPEnabled(ifaceId, "DHCPEnabled", nextv4DHCPState, nextv6DHCPState,
                       asyncResp);
        BMCWEB_LOG_DEBUG << "set DNSEnabled...";
        setDHCPv4Config("DNSEnabled", nextDNS, asyncResp);
        BMCWEB_LOG_DEBUG << "set NTPEnabled...";
        setDHCPv4Config("NTPEnabled", nextNTP, asyncResp);
        BMCWEB_LOG_DEBUG << "set HostNameEnabled...";
        setDHCPv4Config("HostNameEnabled", nextUseDomain, asyncResp);
    }

    boost::container::flat_set<IPv4AddressData>::const_iterator
        getNextStaticIpEntry(
            const boost::container::flat_set<IPv4AddressData>::const_iterator&
                head,
            const boost::container::flat_set<IPv4AddressData>::const_iterator&
                end)
    {
        return std::find_if(head, end, [](const IPv4AddressData& value) {
            return value.origin == "Static";
        });
    }

    boost::container::flat_set<IPv6AddressData>::const_iterator
        getNextStaticIpEntry(
            const boost::container::flat_set<IPv6AddressData>::const_iterator&
                head,
            const boost::container::flat_set<IPv6AddressData>::const_iterator&
                end)
    {
        return std::find_if(head, end, [](const IPv6AddressData& value) {
            return value.origin == "Static";
        });
    }

    void handleIPv4StaticPatch(
        const std::string& ifaceId, nlohmann::json& input,
        const boost::container::flat_set<IPv4AddressData>& ipv4Data,
        const std::shared_ptr<AsyncResp>& asyncResp)
    {
        if ((!input.is_array()) || input.empty())
        {
            messages::propertyValueTypeError(asyncResp->res, input.dump(),
                                             "IPv4StaticAddresses");
            return;
        }

        unsigned entryIdx = 1;
        // Find the first static IP address currently active on the NIC and
        // match it to the first JSON element in the IPv4StaticAddresses array.
        // Match each subsequent JSON element to the next static IP programmed
        // into the NIC.
        boost::container::flat_set<IPv4AddressData>::const_iterator niciPentry =
            getNextStaticIpEntry(ipv4Data.cbegin(), ipv4Data.cend());

        for (nlohmann::json& thisJson : input)
        {
            std::string pathString =
                "IPv4StaticAddresses/" + std::to_string(entryIdx);

            if (!thisJson.is_null() && !thisJson.empty())
            {
                std::optional<std::string> address;
                std::optional<std::string> subnetMask;
                std::optional<std::string> gateway;

                if (!json_util::readJson(thisJson, asyncResp->res, "Address",
                                         address, "SubnetMask", subnetMask,
                                         "Gateway", gateway))
                {
                    messages::propertyValueFormatError(
                        asyncResp->res, thisJson.dump(), pathString);
                    return;
                }

                // Find the address/subnet/gateway values. Any values that are
                // not explicitly provided are assumed to be unmodified from the
                // current state of the interface. Merge existing state into the
                // current request.
                const std::string* addr = nullptr;
                const std::string* gw = nullptr;
                uint8_t prefixLength = 0;
                bool errorInEntry = false;
                if (address)
                {
                    if (ipv4VerifyIpAndGetBitcount(*address))
                    {
                        addr = &(*address);
                    }
                    else
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, *address, pathString + "/Address");
                        errorInEntry = true;
                    }
                }
                else if (niciPentry != ipv4Data.cend())
                {
                    addr = &(niciPentry->address);
                }
                else
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/Address");
                    errorInEntry = true;
                }

                if (subnetMask)
                {
                    if (!ipv4VerifyIpAndGetBitcount(*subnetMask, &prefixLength))
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, *subnetMask,
                            pathString + "/SubnetMask");
                        errorInEntry = true;
                    }
                }
                else if (niciPentry != ipv4Data.cend())
                {
                    if (!ipv4VerifyIpAndGetBitcount(niciPentry->netmask,
                                                    &prefixLength))
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, niciPentry->netmask,
                            pathString + "/SubnetMask");
                        errorInEntry = true;
                    }
                }
                else
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/SubnetMask");
                    errorInEntry = true;
                }

                if (gateway)
                {
                    if (ipv4VerifyIpAndGetBitcount(*gateway))
                    {
                        gw = &(*gateway);
                    }
                    else
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, *gateway, pathString + "/Gateway");
                        errorInEntry = true;
                    }
                }
                else if (niciPentry != ipv4Data.cend())
                {
                    gw = &niciPentry->gateway;
                }
                else
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/Gateway");
                    errorInEntry = true;
                }

                if (errorInEntry)
                {
                    return;
                }

                if (niciPentry != ipv4Data.cend())
                {
                    deleteAndCreateIPv4(ifaceId, niciPentry->id, prefixLength,
                                        *gw, *addr, asyncResp);
                    niciPentry =
                        getNextStaticIpEntry(++niciPentry, ipv4Data.cend());
                }
                else
                {
                    createIPv4(ifaceId, prefixLength, *gateway, *address,
                               asyncResp);
                }
                entryIdx++;
            }
            else
            {
                if (niciPentry == ipv4Data.cend())
                {
                    // Requesting a DELETE/DO NOT MODIFY action for an item
                    // that isn't present on the eth(n) interface. Input JSON is
                    // in error, so bail out.
                    if (thisJson.is_null())
                    {
                        messages::resourceCannotBeDeleted(asyncResp->res);
                        return;
                    }
                    messages::propertyValueFormatError(
                        asyncResp->res, thisJson.dump(), pathString);
                    return;
                }

                if (thisJson.is_null())
                {
                    deleteIPv4(ifaceId, niciPentry->id, asyncResp);
                }
                if (niciPentry != ipv4Data.cend())
                {
                    niciPentry =
                        getNextStaticIpEntry(++niciPentry, ipv4Data.cend());
                }
                entryIdx++;
            }
        }
    }

    void handleStaticNameServersPatch(
        const std::string& ifaceId,
        const std::vector<std::string>& updatedStaticNameServers,
        const std::shared_ptr<AsyncResp>& asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/" + ifaceId,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.EthernetInterface",
            "StaticNameServers",
            std::variant<std::vector<std::string>>{updatedStaticNameServers});
    }

    void handleIPv6StaticAddressesPatch(
        const std::string& ifaceId, const nlohmann::json& input,
        const boost::container::flat_set<IPv6AddressData>& ipv6Data,
        const std::shared_ptr<AsyncResp>& asyncResp)
    {
        if (!input.is_array() || input.empty())
        {
            messages::propertyValueTypeError(asyncResp->res, input.dump(),
                                             "IPv6StaticAddresses");
            return;
        }
        size_t entryIdx = 1;
        boost::container::flat_set<IPv6AddressData>::const_iterator niciPentry =
            getNextStaticIpEntry(ipv6Data.cbegin(), ipv6Data.cend());
        for (const nlohmann::json& thisJson : input)
        {
            std::string pathString =
                "IPv6StaticAddresses/" + std::to_string(entryIdx);

            if (!thisJson.is_null() && !thisJson.empty())
            {
                std::optional<std::string> address;
                std::optional<uint8_t> prefixLength;
                nlohmann::json thisJsonCopy = thisJson;
                if (!json_util::readJson(thisJsonCopy, asyncResp->res,
                                         "Address", address, "PrefixLength",
                                         prefixLength))
                {
                    messages::propertyValueFormatError(
                        asyncResp->res, thisJson.dump(), pathString);
                    return;
                }

                const std::string* addr;
                uint8_t prefix;

                // Find the address and prefixLength values. Any values that are
                // not explicitly provided are assumed to be unmodified from the
                // current state of the interface. Merge existing state into the
                // current request.
                if (address)
                {
                    addr = &(*address);
                }
                else if (niciPentry != ipv6Data.end())
                {
                    addr = &(niciPentry->address);
                }
                else
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/Address");
                    return;
                }

                if (prefixLength)
                {
                    prefix = *prefixLength;
                }
                else if (niciPentry != ipv6Data.end())
                {
                    prefix = niciPentry->prefixLength;
                }
                else
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/PrefixLength");
                    return;
                }

                if (niciPentry != ipv6Data.end())
                {
                    deleteAndCreateIPv6(ifaceId, niciPentry->id, prefix, *addr,
                                        asyncResp);
                    niciPentry =
                        getNextStaticIpEntry(++niciPentry, ipv6Data.cend());
                }
                else
                {
                    createIPv6(ifaceId, *prefixLength, *addr, asyncResp);
                }
                entryIdx++;
            }
            else
            {
                if (niciPentry == ipv6Data.end())
                {
                    // Requesting a DELETE/DO NOT MODIFY action for an item
                    // that isn't present on the eth(n) interface. Input JSON is
                    // in error, so bail out.
                    if (thisJson.is_null())
                    {
                        messages::resourceCannotBeDeleted(asyncResp->res);
                        return;
                    }
                    messages::propertyValueFormatError(
                        asyncResp->res, thisJson.dump(), pathString);
                    return;
                }

                if (thisJson.is_null())
                {
                    deleteIPv6(ifaceId, niciPentry->id, asyncResp);
                }
                if (niciPentry != ipv6Data.cend())
                {
                    niciPentry =
                        getNextStaticIpEntry(++niciPentry, ipv6Data.cend());
                }
                entryIdx++;
            }
        }
    }

    void parseInterfaceData(
        const std::shared_ptr<AsyncResp>& asyncResp, const std::string& ifaceId,
        const EthernetInterfaceData& ethData,
        const boost::container::flat_set<IPv4AddressData>& ipv4Data,
        const boost::container::flat_set<IPv6AddressData>& ipv6Data)
    {
        constexpr const std::array<const char*, 1> inventoryForEthernet = {
            "xyz.openbmc_project.Inventory.Item.Ethernet"};

        nlohmann::json& jsonResponse = asyncResp->res.jsonValue;
        jsonResponse["Id"] = ifaceId;
        jsonResponse["@odata.id"] =
            "/redfish/v1/Managers/bmc/EthernetInterfaces/" + ifaceId;
        jsonResponse["InterfaceEnabled"] = ethData.nicEnabled;

        auto health = std::make_shared<HealthPopulate>(asyncResp);

        crow::connections::systemBus->async_method_call(
            [health](const boost::system::error_code ec,
                     std::vector<std::string>& resp) {
                if (ec)
                {
                    return;
                }

                health->inventory = std::move(resp);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/",
            int32_t(0), inventoryForEthernet);

        health->populate();

        if (ethData.nicEnabled)
        {
            jsonResponse["LinkStatus"] = "LinkUp";
            jsonResponse["Status"]["State"] = "Enabled";
        }
        else
        {
            jsonResponse["LinkStatus"] = "NoLink";
            jsonResponse["Status"]["State"] = "Disabled";
        }

        jsonResponse["LinkStatus"] = ethData.linkUp ? "LinkUp" : "LinkDown";
        jsonResponse["SpeedMbps"] = ethData.speed;
        jsonResponse["MACAddress"] = ethData.mac_address;
        jsonResponse["DHCPv4"]["DHCPEnabled"] =
            translateDHCPEnabledToBool(ethData.DHCPEnabled, true);
        jsonResponse["DHCPv4"]["UseNTPServers"] = ethData.NTPEnabled;
        jsonResponse["DHCPv4"]["UseDNSServers"] = ethData.DNSEnabled;
        jsonResponse["DHCPv4"]["UseDomainName"] = ethData.HostNameEnabled;

        jsonResponse["DHCPv6"]["OperatingMode"] =
            translateDHCPEnabledToBool(ethData.DHCPEnabled, false) ? "Stateful"
                                                                   : "Disabled";
        jsonResponse["DHCPv6"]["UseNTPServers"] = ethData.NTPEnabled;
        jsonResponse["DHCPv6"]["UseDNSServers"] = ethData.DNSEnabled;
        jsonResponse["DHCPv6"]["UseDomainName"] = ethData.HostNameEnabled;

        if (!ethData.hostname.empty())
        {
            jsonResponse["HostName"] = ethData.hostname;

            // When domain name is empty then it means, that it is a network
            // without domain names, and the host name itself must be treated as
            // FQDN
            std::string fqdn = ethData.hostname;
            if (!ethData.domainnames.empty())
            {
                fqdn += "." + ethData.domainnames[0];
            }
            jsonResponse["FQDN"] = fqdn;
        }

        jsonResponse["VLANs"] = {
            {"@odata.id", "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                              ifaceId + "/VLANs"}};

        jsonResponse["NameServers"] = ethData.nameServers;
        jsonResponse["StaticNameServers"] = ethData.staticNameServers;

        nlohmann::json& ipv4Array = jsonResponse["IPv4Addresses"];
        nlohmann::json& ipv4StaticArray = jsonResponse["IPv4StaticAddresses"];
        ipv4Array = nlohmann::json::array();
        ipv4StaticArray = nlohmann::json::array();
        for (auto& ipv4Config : ipv4Data)
        {

            std::string gatewayStr = ipv4Config.gateway;
            if (gatewayStr.empty())
            {
                gatewayStr = "0.0.0.0";
            }

            ipv4Array.push_back({{"AddressOrigin", ipv4Config.origin},
                                 {"SubnetMask", ipv4Config.netmask},
                                 {"Address", ipv4Config.address},
                                 {"Gateway", gatewayStr}});
            if (ipv4Config.origin == "Static")
            {
                ipv4StaticArray.push_back({{"AddressOrigin", ipv4Config.origin},
                                           {"SubnetMask", ipv4Config.netmask},
                                           {"Address", ipv4Config.address},
                                           {"Gateway", gatewayStr}});
            }
        }

        jsonResponse["IPv6DefaultGateway"] = ethData.ipv6_default_gateway;

        nlohmann::json& ipv6Array = jsonResponse["IPv6Addresses"];
        nlohmann::json& ipv6StaticArray = jsonResponse["IPv6StaticAddresses"];
        ipv6Array = nlohmann::json::array();
        ipv6StaticArray = nlohmann::json::array();
        nlohmann::json& ipv6AddrPolicyTable =
            jsonResponse["IPv6AddressPolicyTable"];
        ipv6AddrPolicyTable = nlohmann::json::array();
        for (auto& ipv6Config : ipv6Data)
        {
            ipv6Array.push_back({{"Address", ipv6Config.address},
                                 {"PrefixLength", ipv6Config.prefixLength},
                                 {"AddressOrigin", ipv6Config.origin},
                                 {"AddressState", nullptr}});
            if (ipv6Config.origin == "Static")
            {
                ipv6StaticArray.push_back(
                    {{"Address", ipv6Config.address},
                     {"PrefixLength", ipv6Config.prefixLength},
                     {"AddressOrigin", ipv6Config.origin},
                     {"AddressState", nullptr}});
            }
        }
    }

    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        getEthernetIfaceData(
            params[0],
            [this, asyncResp, ifaceId{std::string(params[0])}](
                const bool& success, const EthernetInterfaceData& ethData,
                const boost::container::flat_set<IPv4AddressData>& ipv4Data,
                const boost::container::flat_set<IPv6AddressData>& ipv6Data) {
                if (!success)
                {
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(asyncResp->res,
                                               "EthernetInterface", ifaceId);
                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#EthernetInterface.v1_4_1.EthernetInterface";
                asyncResp->res.jsonValue["Name"] = "Manager Ethernet Interface";
                asyncResp->res.jsonValue["Description"] =
                    "Management Network Interface";

                parseInterfaceData(asyncResp, ifaceId, ethData, ipv4Data,
                                   ipv6Data);
            });
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& ifaceId = params[0];

        std::optional<std::string> hostname;
        std::optional<std::string> fqdn;
        std::optional<std::string> macAddress;
        std::optional<std::string> ipv6DefaultGateway;
        std::optional<nlohmann::json> ipv4StaticAddresses;
        std::optional<nlohmann::json> ipv6StaticAddresses;
        std::optional<std::vector<std::string>> staticNameServers;
        std::optional<nlohmann::json> dhcpv4;
        std::optional<nlohmann::json> dhcpv6;
        std::optional<bool> interfaceEnabled;
        DHCPParameters v4dhcpParms;
        DHCPParameters v6dhcpParms;

        if (!json_util::readJson(
                req, res, "HostName", hostname, "FQDN", fqdn,
                "IPv4StaticAddresses", ipv4StaticAddresses, "MACAddress",
                macAddress, "StaticNameServers", staticNameServers,
                "IPv6DefaultGateway", ipv6DefaultGateway, "IPv6StaticAddresses",
                ipv6StaticAddresses, "DHCPv4", dhcpv4, "DHCPv6", dhcpv6,
                "InterfaceEnabled", interfaceEnabled))
        {
            return;
        }
        if (dhcpv4)
        {
            if (!json_util::readJson(*dhcpv4, res, "DHCPEnabled",
                                     v4dhcpParms.dhcpv4Enabled, "UseDNSServers",
                                     v4dhcpParms.useDNSServers, "UseNTPServers",
                                     v4dhcpParms.useNTPServers, "UseDomainName",
                                     v4dhcpParms.useUseDomainName))
            {
                return;
            }
        }

        if (dhcpv6)
        {
            if (!json_util::readJson(*dhcpv6, res, "OperatingMode",
                                     v6dhcpParms.dhcpv6OperatingMode,
                                     "UseDNSServers", v6dhcpParms.useDNSServers,
                                     "UseNTPServers", v6dhcpParms.useNTPServers,
                                     "UseDomainName",
                                     v6dhcpParms.useUseDomainName))
            {
                return;
            }
        }

        // Get single eth interface data, and call the below callback for
        // JSON preparation
        getEthernetIfaceData(
            ifaceId,
            [this, asyncResp, ifaceId, hostname = std::move(hostname),
             fqdn = std::move(fqdn), macAddress = std::move(macAddress),
             ipv4StaticAddresses = std::move(ipv4StaticAddresses),
             ipv6DefaultGateway = std::move(ipv6DefaultGateway),
             ipv6StaticAddresses = std::move(ipv6StaticAddresses),
             staticNameServers = std::move(staticNameServers),
             dhcpv4 = std::move(dhcpv4), dhcpv6 = std::move(dhcpv6),
             v4dhcpParms = std::move(v4dhcpParms),
             v6dhcpParms = std::move(v6dhcpParms), interfaceEnabled](
                const bool& success, const EthernetInterfaceData& ethData,
                const boost::container::flat_set<IPv4AddressData>& ipv4Data,
                const boost::container::flat_set<IPv6AddressData>& ipv6Data) {
                if (!success)
                {
                    // ... otherwise return error
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(asyncResp->res,
                                               "Ethernet Interface", ifaceId);
                    return;
                }

                if (dhcpv4 || dhcpv6)
                {
                    handleDHCPPatch(ifaceId, ethData, v4dhcpParms, v6dhcpParms,
                                    asyncResp);
                }

                if (hostname)
                {
                    handleHostnamePatch(*hostname, asyncResp);
                }

                if (fqdn)
                {
                    handleFqdnPatch(ifaceId, *fqdn, asyncResp);
                }

                if (macAddress)
                {
                    handleMACAddressPatch(ifaceId, *macAddress, asyncResp);
                }

                if (ipv4StaticAddresses)
                {
                    // TODO(ed) for some reason the capture of ipv4Addresses
                    // above is returning a const value, not a non-const
                    // value. This doesn't really work for us, as we need to
                    // be able to efficiently move out the intermedia
                    // nlohmann::json objects. This makes a copy of the
                    // structure, and operates on that, but could be done
                    // more efficiently
                    nlohmann::json ipv4Static = *ipv4StaticAddresses;
                    handleIPv4StaticPatch(ifaceId, ipv4Static, ipv4Data,
                                          asyncResp);
                }

                if (staticNameServers)
                {
                    handleStaticNameServersPatch(ifaceId, *staticNameServers,
                                                 asyncResp);
                }

                if (ipv6DefaultGateway)
                {
                    messages::propertyNotWritable(asyncResp->res,
                                                  "IPv6DefaultGateway");
                }

                if (ipv6StaticAddresses)
                {
                    nlohmann::json ipv6Static = *ipv6StaticAddresses;
                    handleIPv6StaticAddressesPatch(ifaceId, ipv6Static,
                                                   ipv6Data, asyncResp);
                }

                if (interfaceEnabled)
                {
                    setEthernetInterfaceBoolProperty(
                        ifaceId, "NICEnabled", *interfaceEnabled, asyncResp);
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
    VlanNetworkInterface(App& app) :
        Node(app,
             "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/<str>/",
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
    void parseInterfaceData(nlohmann::json& jsonResponse,
                            const std::string& parentIfaceId,
                            const std::string& ifaceId,
                            const EthernetInterfaceData& ethData)
    {
        // Fill out obvious data...
        jsonResponse["Id"] = ifaceId;
        jsonResponse["@odata.id"] =
            "/redfish/v1/Managers/bmc/EthernetInterfaces/" + parentIfaceId +
            "/VLANs/" + ifaceId;

        jsonResponse["VLANEnable"] = true;
        if (!ethData.vlan_id.empty())
        {
            jsonResponse["VLANId"] = ethData.vlan_id.back();
        }
    }

    bool verifyNames(const std::string& parent, const std::string& iface)
    {
        if (!boost::starts_with(iface, parent + "_"))
        {
            return false;
        }
        return true;
    }

    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
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

        const std::string& parentIfaceId = params[0];
        const std::string& ifaceId = params[1];
        res.jsonValue["@odata.type"] =
            "#VLanNetworkInterface.v1_1_0.VLanNetworkInterface";
        res.jsonValue["Name"] = "VLAN Network Interface";

        if (!verifyNames(parentIfaceId, ifaceId))
        {
            return;
        }

        // Get single eth interface data, and call the below callback for
        // JSON preparation
        getEthernetIfaceData(
            params[1],
            [this, asyncResp, parentIfaceId{std::string(params[0])},
             ifaceId{std::string(params[1])}](
                const bool& success, const EthernetInterfaceData& ethData,
                const boost::container::flat_set<IPv4AddressData>&,
                const boost::container::flat_set<IPv6AddressData>&) {
                if (success && ethData.vlan_id.size() != 0)
                {
                    parseInterfaceData(asyncResp->res.jsonValue, parentIfaceId,
                                       ifaceId, ethData);
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

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& parentIfaceId = params[0];
        const std::string& ifaceId = params[1];

        if (!verifyNames(parentIfaceId, ifaceId))
        {
            messages::resourceNotFound(asyncResp->res, "VLAN Network Interface",
                                       ifaceId);
            return;
        }

        bool vlanEnable = false;
        uint32_t vlanId = 0;

        if (!json_util::readJson(req, res, "VLANEnable", vlanEnable, "VLANId",
                                 vlanId))
        {
            return;
        }

        // Get single eth interface data, and call the below callback for
        // JSON preparation
        getEthernetIfaceData(
            params[1],
            [asyncResp, parentIfaceId{std::string(params[0])},
             ifaceId{std::string(params[1])}, &vlanEnable,
             &vlanId](const bool& success, const EthernetInterfaceData& ethData,
                      const boost::container::flat_set<IPv4AddressData>&,
                      const boost::container::flat_set<IPv6AddressData>&) {
                if (success && !ethData.vlan_id.empty())
                {
                    auto callback =
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                messages::internalError(asyncResp->res);
                            }
                        };

                    if (vlanEnable == true)
                    {
                        crow::connections::systemBus->async_method_call(
                            std::move(callback), "xyz.openbmc_project.Network",
                            "/xyz/openbmc_project/network/" + ifaceId,
                            "org.freedesktop.DBus.Properties", "Set",
                            "xyz.openbmc_project.Network.VLAN", "Id",
                            std::variant<uint32_t>(vlanId));
                    }
                    else
                    {
                        BMCWEB_LOG_DEBUG << "vlanEnable is false. Deleting the "
                                            "vlan interface";
                        crow::connections::systemBus->async_method_call(
                            std::move(callback), "xyz.openbmc_project.Network",
                            std::string("/xyz/openbmc_project/network/") +
                                ifaceId,
                            "xyz.openbmc_project.Object.Delete", "Delete");
                    }
                }
                else
                {
                    // TODO(Pawel)consider distinguish between non existing
                    // object, and other errors
                    messages::resourceNotFound(
                        asyncResp->res, "VLAN Network Interface", ifaceId);
                    return;
                }
            });
    }

    void doDelete(crow::Response& res, const crow::Request&,
                  const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& parentIfaceId = params[0];
        const std::string& ifaceId = params[1];

        if (!verifyNames(parentIfaceId, ifaceId))
        {
            messages::resourceNotFound(asyncResp->res, "VLAN Network Interface",
                                       ifaceId);
            return;
        }

        // Get single eth interface data, and call the below callback for
        // JSON preparation
        getEthernetIfaceData(
            params[1],
            [asyncResp, parentIfaceId{std::string(params[0])},
             ifaceId{std::string(params[1])}](
                const bool& success, const EthernetInterfaceData& ethData,
                const boost::container::flat_set<IPv4AddressData>&,
                const boost::container::flat_set<IPv6AddressData>&) {
                if (success && !ethData.vlan_id.empty())
                {
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
    VlanNetworkInterfaceCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            // This means there is a problem with the router
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& rootInterfaceName = params[0];

        // Get eth interface list, and call the below callback for JSON
        // preparation
        getEthernetIfaceList(
            [asyncResp, rootInterfaceName{std::string(rootInterfaceName)}](
                const bool& success,
                const boost::container::flat_set<std::string>& ifaceList) {
                if (!success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                if (ifaceList.find(rootInterfaceName) == ifaceList.end())
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "VLanNetworkInterfaceCollection",
                                               rootInterfaceName);
                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#VLanNetworkInterfaceCollection."
                    "VLanNetworkInterfaceCollection";
                asyncResp->res.jsonValue["Name"] =
                    "VLAN Network Interface Collection";

                nlohmann::json ifaceArray = nlohmann::json::array();

                for (const std::string& ifaceItem : ifaceList)
                {
                    if (boost::starts_with(ifaceItem, rootInterfaceName + "_"))
                    {
                        std::string path =
                            "/redfish/v1/Managers/bmc/EthernetInterfaces/";
                        path += rootInterfaceName;
                        path += "/VLANs/";
                        path += ifaceItem;
                        ifaceArray.push_back({{"@odata.id", std::move(path)}});
                    }
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    ifaceArray.size();
                asyncResp->res.jsonValue["Members"] = std::move(ifaceArray);
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                    rootInterfaceName + "/VLANs";
            });
    }

    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        bool vlanEnable = false;
        uint32_t vlanId = 0;
        if (!json_util::readJson(req, res, "VLANId", vlanId, "VLANEnable",
                                 vlanEnable))
        {
            return;
        }
        // Need both vlanId and vlanEnable to service this request
        if (!vlanId)
        {
            messages::propertyMissing(asyncResp->res, "VLANId");
        }
        if (!vlanEnable)
        {
            messages::propertyMissing(asyncResp->res, "VLANEnable");
        }
        if (static_cast<bool>(vlanId) ^ vlanEnable)
        {
            return;
        }

        const std::string& rootInterfaceName = params[0];
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
