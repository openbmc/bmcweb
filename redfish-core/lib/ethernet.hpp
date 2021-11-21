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

#include <app_class_decl.hpp>
#include "health_class_decl.hpp"
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <error_messages.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/json_utils.hpp>
#include <boost/algorithm/string.hpp>

using crow::App;

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

bool extractEthernetInterfaceData(const std::string& ethifaceId,
                                         GetManagedObjects& dbusData,
                                         EthernetInterfaceData& ethData);

// Helper function that extracts data for single ethernet ipv6 address
void extractIPV6Data(const std::string& ethifaceId,
                    const GetManagedObjects& dbusData,
                    boost::container::flat_set<IPv6AddressData>& ipv6Config);

// Helper function that extracts data for single ethernet ipv4 address
void
    extractIPData(const std::string& ethifaceId,
                  const GetManagedObjects& dbusData,
                  boost::container::flat_set<IPv4AddressData>& ipv4Config);

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
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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

inline void updateIPv4DefaultGateway(
    const std::string& ifaceId, const std::string& gateway,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.result(boost::beast::http::status::no_content);
        },
        "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.EthernetInterface", "DefaultGateway",
        std::variant<std::string>(gateway));
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
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    auto createIpHandler = [asyncResp, ifaceId,
                            gateway](const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        updateIPv4DefaultGateway(ifaceId, gateway, asyncResp);
    };

    crow::connections::systemBus->async_method_call(
        std::move(createIpHandler), "xyz.openbmc_project.Network",
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
inline void
    deleteAndCreateIPv4(const std::string& ifaceId, const std::string& id,
                        uint8_t prefixLength, const std::string& gateway,
                        const std::string& address,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, ifaceId, address, prefixLength,
         gateway](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp, ifaceId,
                 gateway](const boost::system::error_code ec2) {
                    if (ec2)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    updateIPv4DefaultGateway(ifaceId, gateway, asyncResp);
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
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
inline void
    deleteAndCreateIPv6(const std::string& ifaceId, const std::string& id,
                        uint8_t prefixLength, const std::string& address,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
                    (ipv4.origin == "DHCP") || (ipv4.origin == "Static"))
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
                        std::string ifaceId = objpath.first.filename();
                        if (ifaceId.empty())
                        {
                            continue;
                        }
                        // and put it into output vector.
                        ifaceList.emplace(ifaceId);
                    }
                }
            }
            // Finally make a callback with useful data
            callback(true, ifaceList);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void
    handleHostnamePatch(const std::string& hostname,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network/config",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
        std::variant<std::string>(hostname));
}

inline void
    handleDomainnamePatch(const std::string& ifaceId,
                          const std::string& domainname,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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

inline bool isHostnameValid(const std::string& hostname)
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

inline bool isDomainnameValid(const std::string& domainname)
{
    // Can have multiple subdomains
    // Top Level Domain's min length is 2 character
    const std::regex pattern("^([A-Za-z0-9][a-zA-Z0-9\\-]{1,61}|[a-zA-Z0-9]"
                             "{1,30}\\.)*[a-zA-Z]{2,}$");

    return std::regex_match(domainname, pattern);
}

inline void handleFqdnPatch(const std::string& ifaceId, const std::string& fqdn,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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

inline void
    handleMACAddressPatch(const std::string& ifaceId,
                          const std::string& macAddress,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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

inline void setDHCPEnabled(const std::string& ifaceId,
                           const std::string& propertyName, const bool v4Value,
                           const bool v6Value,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
            messages::success(asyncResp->res);
        },
        "xyz.openbmc_project.Network",
        "/xyz/openbmc_project/network/" + ifaceId,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.EthernetInterface", propertyName,
        std::variant<std::string>{dhcp});
}

inline void setEthernetInterfaceBoolProperty(
    const std::string& ifaceId, const std::string& propertyName,
    const bool& value, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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

inline void setDHCPv4Config(const std::string& propertyName, const bool& value,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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

inline void handleDHCPPatch(const std::string& ifaceId,
                            const EthernetInterfaceData& ethData,
                            const DHCPParameters& v4dhcpParms,
                            const DHCPParameters& v6dhcpParms,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    bool ipv4Active = translateDHCPEnabledToBool(ethData.DHCPEnabled, true);
    bool ipv6Active = translateDHCPEnabledToBool(ethData.DHCPEnabled, false);

    bool nextv4DHCPState =
        v4dhcpParms.dhcpv4Enabled ? *v4dhcpParms.dhcpv4Enabled : ipv4Active;

    bool nextv6DHCPState{};
    if (v6dhcpParms.dhcpv6OperatingMode)
    {
        if ((*v6dhcpParms.dhcpv6OperatingMode != "Stateful") &&
            (*v6dhcpParms.dhcpv6OperatingMode != "Stateless") &&
            (*v6dhcpParms.dhcpv6OperatingMode != "Disabled"))
        {
            messages::propertyValueFormatError(asyncResp->res,
                                               *v6dhcpParms.dhcpv6OperatingMode,
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

inline boost::container::flat_set<IPv4AddressData>::const_iterator
    getNextStaticIpEntry(
        const boost::container::flat_set<IPv4AddressData>::const_iterator& head,
        const boost::container::flat_set<IPv4AddressData>::const_iterator& end)
{
    return std::find_if(head, end, [](const IPv4AddressData& value) {
        return value.origin == "Static";
    });
}

inline boost::container::flat_set<IPv6AddressData>::const_iterator
    getNextStaticIpEntry(
        const boost::container::flat_set<IPv6AddressData>::const_iterator& head,
        const boost::container::flat_set<IPv6AddressData>::const_iterator& end)
{
    return std::find_if(head, end, [](const IPv6AddressData& value) {
        return value.origin == "Static";
    });
}

inline void handleIPv4StaticPatch(
    const std::string& ifaceId, nlohmann::json& input,
    const boost::container::flat_set<IPv4AddressData>& ipv4Data,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if ((!input.is_array()) || input.empty())
    {
        messages::propertyValueTypeError(
            asyncResp->res,
            input.dump(2, ' ', true, nlohmann::json::error_handler_t::replace),
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
                    asyncResp->res,
                    thisJson.dump(2, ' ', true,
                                  nlohmann::json::error_handler_t::replace),
                    pathString);
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
                    messages::propertyValueFormatError(asyncResp->res, *address,
                                                       pathString + "/Address");
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
                    messages::propertyValueFormatError(asyncResp->res, *gateway,
                                                       pathString + "/Gateway");
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
                deleteAndCreateIPv4(ifaceId, niciPentry->id, prefixLength, *gw,
                                    *addr, asyncResp);
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
                    asyncResp->res,
                    thisJson.dump(2, ' ', true,
                                  nlohmann::json::error_handler_t::replace),
                    pathString);
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

inline void handleStaticNameServersPatch(
    const std::string& ifaceId,
    const std::vector<std::string>& updatedStaticNameServers,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
        "xyz.openbmc_project.Network.EthernetInterface", "StaticNameServers",
        std::variant<std::vector<std::string>>{updatedStaticNameServers});
}

inline void handleIPv6StaticAddressesPatch(
    const std::string& ifaceId, const nlohmann::json& input,
    const boost::container::flat_set<IPv6AddressData>& ipv6Data,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!input.is_array() || input.empty())
    {
        messages::propertyValueTypeError(
            asyncResp->res,
            input.dump(2, ' ', true, nlohmann::json::error_handler_t::replace),
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
            if (!json_util::readJson(thisJsonCopy, asyncResp->res, "Address",
                                     address, "PrefixLength", prefixLength))
            {
                messages::propertyValueFormatError(
                    asyncResp->res,
                    thisJson.dump(2, ' ', true,
                                  nlohmann::json::error_handler_t::replace),
                    pathString);
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
                    asyncResp->res,
                    thisJson.dump(2, ' ', true,
                                  nlohmann::json::error_handler_t::replace),
                    pathString);
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

inline void parseInterfaceData(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& ifaceId, const EthernetInterfaceData& ethData,
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
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", "/", int32_t(0),
        inventoryForEthernet);

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
        {"@odata.id",
         "/redfish/v1/Managers/bmc/EthernetInterfaces/" + ifaceId + "/VLANs"}};

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

    std::string ipv6GatewayStr = ethData.ipv6_default_gateway;
    if (ipv6GatewayStr.empty())
    {
        ipv6GatewayStr = "0:0:0:0:0:0:0:0";
    }

    jsonResponse["IPv6DefaultGateway"] = ipv6GatewayStr;

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

inline void parseInterfaceData(nlohmann::json& jsonResponse,
                               const std::string& parentIfaceId,
                               const std::string& ifaceId,
                               const EthernetInterfaceData& ethData)
{
    // Fill out obvious data...
    jsonResponse["Id"] = ifaceId;
    jsonResponse["@odata.id"] = "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                                parentIfaceId + "/VLANs/" + ifaceId;

    jsonResponse["VLANEnable"] = true;
    if (!ethData.vlan_id.empty())
    {
        jsonResponse["VLANId"] = ethData.vlan_id.back();
    }
}

inline bool verifyNames(const std::string& parent, const std::string& iface)
{
    if (!boost::starts_with(iface, parent + "_"))
    {
        return false;
    }
    return true;
}

void requestEthernetInterfacesRoutes(App& app);

} // namespace redfish
