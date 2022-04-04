#pragma once

#include <app.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <query.hpp>
#include <registries/privilege_registry.hpp>
#include <sdbusplus/asio/property.hpp>
#include <utils/json_utils.hpp>

#include <optional>
#include <utility>

// TODO(ed) requestRoutesHypervisorSystems seems to have copy-pasted a
// lot of code, and has a number of methods that have name conflicts with the
// normal ethernet internfaces in ethernet.hpp.  For the moment, we'll put
// hypervisor in a namespace to isolate it, but these methods eventually need
// deduplicated
namespace redfish::hypervisor
{

/**
 * @brief Retrieves hypervisor state properties over dbus
 *
 * The hypervisor state object is optional so this function will only set the
 * state variables if the object is found
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void getHypervisorState(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get hypervisor state information.";
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.State.Hypervisor",
        "/xyz/openbmc_project/state/hypervisor0",
        "xyz.openbmc_project.State.Host", "CurrentHostState",
        [aResp](const boost::system::error_code ec,
                const std::string& hostState) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // This is an optional D-Bus object so just return if
                // error occurs
                return;
            }

            BMCWEB_LOG_DEBUG << "Hypervisor state: " << hostState;
            // Verify Host State
            if (hostState == "xyz.openbmc_project.State.Host.HostState.Running")
            {
                aResp->res.jsonValue["PowerState"] = "On";
                aResp->res.jsonValue["Status"]["State"] = "Enabled";
            }
            else if (hostState == "xyz.openbmc_project.State.Host.HostState."
                                  "Quiesced")
            {
                aResp->res.jsonValue["PowerState"] = "On";
                aResp->res.jsonValue["Status"]["State"] = "Quiesced";
            }
            else if (hostState == "xyz.openbmc_project.State.Host.HostState."
                                  "Standby")
            {
                aResp->res.jsonValue["PowerState"] = "On";
                aResp->res.jsonValue["Status"]["State"] = "StandbyOffline";
            }
            else if (hostState == "xyz.openbmc_project.State.Host.HostState."
                                  "TransitioningToRunning")
            {
                aResp->res.jsonValue["PowerState"] = "PoweringOn";
                aResp->res.jsonValue["Status"]["State"] = "Starting";
            }
            else if (hostState == "xyz.openbmc_project.State.Host.HostState."
                                  "TransitioningToOff")
            {
                aResp->res.jsonValue["PowerState"] = "PoweringOff";
                aResp->res.jsonValue["Status"]["State"] = "Enabled";
            }
            else if (hostState ==
                     "xyz.openbmc_project.State.Host.HostState.Off")
            {
                aResp->res.jsonValue["PowerState"] = "Off";
                aResp->res.jsonValue["Status"]["State"] = "Disabled";
            }
            else
            {
                messages::internalError(aResp->res);
                return;
            }
        });
}

/**
 * @brief Populate Actions if any are valid for hypervisor object
 *
 * The hypervisor state object is optional so this function will only set the
 * Action if the object is found
 *
 * @param[in] aResp     Shared pointer for completing asynchronous calls.
 *
 * @return None.
 */
inline void
    getHypervisorActions(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    BMCWEB_LOG_DEBUG << "Get hypervisor actions.";
    crow::connections::systemBus->async_method_call(
        [aResp](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                objInfo) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // This is an optional D-Bus object so just return if
                // error occurs
                return;
            }

            if (objInfo.empty())
            {
                // As noted above, this is an optional interface so just return
                // if there is no instance found
                return;
            }

            if (objInfo.size() > 1)
            {
                // More then one hypervisor object is not supported and is an
                // error
                messages::internalError(aResp->res);
                return;
            }

            // Object present so system support limited ComputerSystem Action
            aResp->res.jsonValue["Actions"]["#ComputerSystem.Reset"] = {
                {"target",
                 "/redfish/v1/Systems/hypervisor/Actions/ComputerSystem.Reset"},
                {"@Redfish.ActionInfo",
                 "/redfish/v1/Systems/hypervisor/ResetActionInfo"}};
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/state/hypervisor0",
        std::array<const char*, 1>{"xyz.openbmc_project.State.Host"});
}

inline bool extractHypervisorInterfaceData(
    const std::string& ethIfaceId,
    const dbus::utility::ManagedObjectType& dbusData,
    EthernetInterfaceData& ethData,
    boost::container::flat_set<IPv4AddressData>& ipv4Config)
{
    bool idFound = false;
    for (const auto& objpath : dbusData)
    {
        for (const auto& ifacePair : objpath.second)
        {
            if (objpath.first ==
                "/xyz/openbmc_project/network/hypervisor/" + ethIfaceId)
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
                else if (ifacePair.first ==
                         "xyz.openbmc_project.Network.EthernetInterface")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "DHCPEnabled")
                        {
                            const std::string* dhcp =
                                std::get_if<std::string>(&propertyPair.second);
                            if (dhcp != nullptr)
                            {
                                ethData.DHCPEnabled = *dhcp;
                                break; // Interested on only "DHCPEnabled".
                                       // Stop parsing since we got the
                                       // "DHCPEnabled" value.
                            }
                        }
                    }
                }
            }
            if (objpath.first == "/xyz/openbmc_project/network/hypervisor/" +
                                     ethIfaceId + "/ipv4/addr0")
            {
                std::pair<boost::container::flat_set<IPv4AddressData>::iterator,
                          bool>
                    it = ipv4Config.insert(IPv4AddressData{});
                IPv4AddressData& ipv4Address = *it.first;
                if (ifacePair.first == "xyz.openbmc_project.Object.Enable")
                {
                    for (const auto& property : ifacePair.second)
                    {
                        if (property.first == "Enabled")
                        {
                            const bool* intfEnable =
                                std::get_if<bool>(&property.second);
                            if (intfEnable != nullptr)
                            {
                                ipv4Address.isActive = *intfEnable;
                                break;
                            }
                        }
                    }
                }
                if (ifacePair.first == "xyz.openbmc_project.Network.IP")
                {
                    for (const auto& property : ifacePair.second)
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
                        else if (property.first == "Type" ||
                                 property.first == "Gateway")
                        {
                            // Type & Gateway is not used
                            continue;
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
            if (objpath.first == "/xyz/openbmc_project/network/hypervisor")
            {
                // System configuration shows up in the global namespace, so no
                // need to check eth number
                if (ifacePair.first ==
                    "xyz.openbmc_project.Network.SystemConfiguration")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "HostName")
                        {
                            const std::string* hostName =
                                std::get_if<std::string>(&propertyPair.second);
                            if (hostName != nullptr)
                            {
                                ethData.hostname = *hostName;
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
                    }
                }
            }
        }
    }
    return idFound;
}
/**
 * Function that retrieves all properties for given Hypervisor Ethernet
 * Interface Object from Settings Manager
 * @param ethIfaceId Hypervisor ethernet interface id to query on DBus
 * @param callback a function that shall be called to convert Dbus output
 * into JSON
 */
template <typename CallbackFunc>
void getHypervisorIfaceData(const std::string& ethIfaceId,
                            CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [ethIfaceId{std::string{ethIfaceId}},
         callback{std::forward<CallbackFunc>(callback)}](
            const boost::system::error_code error,
            const dbus::utility::ManagedObjectType& resp) {
            EthernetInterfaceData ethData{};
            boost::container::flat_set<IPv4AddressData> ipv4Data;
            if (error)
            {
                callback(false, ethData, ipv4Data);
                return;
            }

            bool found = extractHypervisorInterfaceData(ethIfaceId, resp,
                                                        ethData, ipv4Data);
            if (!found)
            {
                BMCWEB_LOG_INFO << "Hypervisor Interface not found";
            }
            callback(found, ethData, ipv4Data);
        },
        "xyz.openbmc_project.Settings", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
 * @brief Sets the Hypervisor Interface IPAddress DBUS
 *
 * @param[in] aResp          Shared pointer for generating response message.
 * @param[in] ipv4Address    Address from the incoming request
 * @param[in] ethIfaceId     Hypervisor Interface Id
 *
 * @return None.
 */
inline void
    setHypervisorIPv4Address(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& ethIfaceId,
                             const std::string& ipv4Address)
{
    BMCWEB_LOG_DEBUG << "Setting the Hypervisor IPaddress : " << ipv4Address
                     << " on Iface: " << ethIfaceId;
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                return;
            }
            BMCWEB_LOG_DEBUG << "Hypervisor IPaddress is Set";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/network/hypervisor/" + ethIfaceId + "/ipv4/addr0",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "Address",
        dbus::utility::DbusVariantType(ipv4Address));
}

/**
 * @brief Sets the Hypervisor Interface SubnetMask DBUS
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] subnet    SubnetMask from the incoming request
 * @param[in] ethIfaceId Hypervisor Interface Id
 *
 * @return None.
 */
inline void
    setHypervisorIPv4Subnet(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                            const std::string& ethIfaceId, const uint8_t subnet)
{
    BMCWEB_LOG_DEBUG << "Setting the Hypervisor subnet : " << subnet
                     << " on Iface: " << ethIfaceId;

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                return;
            }
            BMCWEB_LOG_DEBUG << "SubnetMask is Set";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/network/hypervisor/" + ethIfaceId + "/ipv4/addr0",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "PrefixLength",
        dbus::utility::DbusVariantType(subnet));
}

/**
 * @brief Sets the Hypervisor Interface Gateway DBUS
 *
 * @param[in] aResp          Shared pointer for generating response message.
 * @param[in] gateway        Gateway from the incoming request
 * @param[in] ethIfaceId     Hypervisor Interface Id
 *
 * @return None.
 */
inline void
    setHypervisorIPv4Gateway(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const std::string& gateway)
{
    BMCWEB_LOG_DEBUG
        << "Setting the DefaultGateway to the last configured gateway";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                return;
            }
            BMCWEB_LOG_DEBUG << "Default Gateway is Set";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/network/hypervisor",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.SystemConfiguration", "DefaultGateway",
        dbus::utility::DbusVariantType(gateway));
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
inline void
    createHypervisorIPv4(const std::string& ifaceId, uint8_t prefixLength,
                         const std::string& gateway, const std::string& address,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    setHypervisorIPv4Address(asyncResp, ifaceId, address);
    setHypervisorIPv4Gateway(asyncResp, gateway);
    setHypervisorIPv4Subnet(asyncResp, ifaceId, prefixLength);
}

/**
 * @brief Deletes given IPv4 interface
 *
 * @param[in] ifaceId     Id of interface whose IP should be deleted
 * @param[io] asyncResp   Response object that will be returned to client
 *
 * @return None
 */
inline void
    deleteHypervisorIPv4(const std::string& ifaceId,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string address = "0.0.0.0";
    std::string gateway = "0.0.0.0";
    const uint8_t prefixLength = 0;
    setHypervisorIPv4Address(asyncResp, ifaceId, address);
    setHypervisorIPv4Gateway(asyncResp, gateway);
    setHypervisorIPv4Subnet(asyncResp, ifaceId, prefixLength);
}

inline void parseInterfaceData(
    nlohmann::json& jsonResponse, const std::string& ifaceId,
    const EthernetInterfaceData& ethData,
    const boost::container::flat_set<IPv4AddressData>& ipv4Data)
{
    jsonResponse["Id"] = ifaceId;
    jsonResponse["@odata.id"] =
        "/redfish/v1/Systems/hypervisor/EthernetInterfaces/" + ifaceId;
    jsonResponse["InterfaceEnabled"] = true;
    jsonResponse["MACAddress"] = ethData.mac_address;

    jsonResponse["HostName"] = ethData.hostname;
    jsonResponse["DHCPv4"]["DHCPEnabled"] =
        translateDHCPEnabledToBool(ethData.DHCPEnabled, true);

    nlohmann::json& ipv4Array = jsonResponse["IPv4Addresses"];
    nlohmann::json& ipv4StaticArray = jsonResponse["IPv4StaticAddresses"];
    ipv4Array = nlohmann::json::array();
    ipv4StaticArray = nlohmann::json::array();
    for (const auto& ipv4Config : ipv4Data)
    {
        if (ipv4Config.isActive)
        {

            ipv4Array.push_back({{"AddressOrigin", ipv4Config.origin},
                                 {"SubnetMask", ipv4Config.netmask},
                                 {"Address", ipv4Config.address},
                                 {"Gateway", ethData.default_gateway}});
            if (ipv4Config.origin == "Static")
            {
                ipv4StaticArray.push_back(
                    {{"AddressOrigin", ipv4Config.origin},
                     {"SubnetMask", ipv4Config.netmask},
                     {"Address", ipv4Config.address},
                     {"Gateway", ethData.default_gateway}});
            }
        }
    }
}

inline void setDHCPEnabled(const std::string& ifaceId,
                           const bool& ipv4DHCPEnabled,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const std::string dhcp = getDhcpEnabledEnumeration(ipv4DHCPEnabled, false);
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/network/hypervisor/" + ifaceId,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.EthernetInterface", "DHCPEnabled",
        dbus::utility::DbusVariantType{dhcp});

    // Set the IPv4 address origin to the DHCP / Static as per the new value
    // of the DHCPEnabled property
    std::string origin;
    if (!ipv4DHCPEnabled)
    {
        origin = "xyz.openbmc_project.Network.IP.AddressOrigin.Static";
    }
    else
    {
        // DHCPEnabled is set to true. Delete the current IPv4 settings
        // to receive the new values from DHCP server.
        deleteHypervisorIPv4(ifaceId, asyncResp);
        origin = "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP";
    }
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec;
                messages::internalError(asyncResp->res);
                return;
            }
            BMCWEB_LOG_DEBUG << "Hypervisor IPaddress Origin is Set";
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/network/hypervisor/" + ifaceId + "/ipv4/addr0",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "Origin",
        dbus::utility::DbusVariantType(origin));
}

inline void handleHypervisorIPv4StaticPatch(
    const std::string& ifaceId, const nlohmann::json& input,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if ((!input.is_array()) || input.empty())
    {
        messages::propertyValueTypeError(asyncResp->res, input.dump(),
                                         "IPv4StaticAddresses");
        return;
    }

    // Hypervisor considers the first IP address in the array list
    // as the Hypervisor's virtual management interface supports single IPv4
    // address
    const nlohmann::json& thisJson = input[0];

    // For the error string
    std::string pathString = "IPv4StaticAddresses/1";

    if (!thisJson.is_null() && !thisJson.empty())
    {
        std::optional<std::string> address;
        std::optional<std::string> subnetMask;
        std::optional<std::string> gateway;
        nlohmann::json thisJsonCopy = thisJson;
        if (!json_util::readJson(thisJsonCopy, asyncResp->res, "Address",
                                 address, "SubnetMask", subnetMask, "Gateway",
                                 gateway))
        {
            messages::propertyValueFormatError(
                asyncResp->res,
                thisJson.dump(2, ' ', true,
                              nlohmann::json::error_handler_t::replace),
                pathString);
            return;
        }

        uint8_t prefixLength = 0;
        bool errorInEntry = false;
        if (address)
        {
            if (!ipv4VerifyIpAndGetBitcount(*address))
            {
                messages::propertyValueFormatError(asyncResp->res, *address,
                                                   pathString + "/Address");
                errorInEntry = true;
            }
        }
        else
        {
            messages::propertyMissing(asyncResp->res, pathString + "/Address");
            errorInEntry = true;
        }

        if (subnetMask)
        {
            if (!ipv4VerifyIpAndGetBitcount(*subnetMask, &prefixLength))
            {
                messages::propertyValueFormatError(asyncResp->res, *subnetMask,
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
            if (!ipv4VerifyIpAndGetBitcount(*gateway))
            {
                messages::propertyValueFormatError(asyncResp->res, *gateway,
                                                   pathString + "/Gateway");
                errorInEntry = true;
            }
        }
        else
        {
            messages::propertyMissing(asyncResp->res, pathString + "/Gateway");
            errorInEntry = true;
        }

        if (errorInEntry)
        {
            return;
        }

        BMCWEB_LOG_DEBUG << "Calling createHypervisorIPv4 on : " << ifaceId
                         << "," << *address;
        createHypervisorIPv4(ifaceId, prefixLength, *gateway, *address,
                             asyncResp);
        // Set the DHCPEnabled to false since the Static IPv4 is set
        setDHCPEnabled(ifaceId, false, asyncResp);
    }
    else
    {
        if (thisJson.is_null())
        {
            deleteHypervisorIPv4(ifaceId, asyncResp);
        }
    }
}

inline void
    handleHostnamePatch(const std::string& hostName,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!isHostnameValid(hostName))
    {
        messages::propertyValueFormatError(asyncResp->res, hostName,
                                           "HostName");
        return;
    }

    asyncResp->res.jsonValue["HostName"] = hostName;
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
        },
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/network/hypervisor",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
        dbus::utility::DbusVariantType(hostName));
}

inline void
    setIPv4InterfaceEnabled(const std::string& ifaceId, const bool& isActive,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
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
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/network/hypervisor/" + ifaceId + "/ipv4/addr0",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Object.Enable", "Enabled",
        dbus::utility::DbusVariantType(isActive));
}

inline void requestRoutesHypervisorSystems(App& app)
{
    /**
     * Hypervisor Systems derived class for delivering Computer Systems Schema.
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/hypervisor/")
        .privileges(redfish::privileges::getComputerSystem)
        .methods(boost::beast::http::verb::get)([&app](const crow::Request& req,
                                                       const std::shared_ptr<
                                                           bmcweb::AsyncResp>&
                                                           asyncResp) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
            {
                return;
            }
            sdbusplus::asio::getProperty<std::string>(
                *crow::connections::systemBus, "xyz.openbmc_project.Settings",
                "/xyz/openbmc_project/network/hypervisor",
                "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
                [asyncResp](const boost::system::error_code ec,
                            const std::string& /*hostName*/) {
                    if (ec)
                    {
                        messages::resourceNotFound(asyncResp->res, "System",
                                                   "hypervisor");
                        return;
                    }
                    BMCWEB_LOG_DEBUG << "Hypervisor is available";

                    asyncResp->res.jsonValue["@odata.type"] =
                        "#ComputerSystem.v1_6_0.ComputerSystem";
                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/Systems/hypervisor";
                    asyncResp->res.jsonValue["Description"] = "Hypervisor";
                    asyncResp->res.jsonValue["Name"] = "Hypervisor";
                    asyncResp->res.jsonValue["Id"] = "hypervisor";
                    asyncResp->res.jsonValue["SystemType"] = "OS";
                    asyncResp->res.jsonValue["Links"]["ManagedBy"] = {
                        {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
                    asyncResp->res.jsonValue["EthernetInterfaces"] = {
                        {"@odata.id",
                         "/redfish/v1/Systems/hypervisor/EthernetInterfaces"}};
                    getHypervisorState(asyncResp);
                    getHypervisorActions(asyncResp);
                    // TODO: Add "SystemType" : "hypervisor"
                });
        });

    /**
     * HypervisorInterfaceCollection class to handle the GET and PATCH on
     * Hypervisor Interface
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/hypervisor/EthernetInterfaces/")
        .privileges(redfish::privileges::getEthernetInterfaceCollection)
        .methods(boost::beast::http::verb::get)([&app](const crow::Request& req,
                                                       const std::shared_ptr<
                                                           bmcweb::AsyncResp>&
                                                           asyncResp) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
            {
                return;
            }
            const std::array<const char*, 1> interfaces = {
                "xyz.openbmc_project.Network.EthernetInterface"};

            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code error,
                            const dbus::utility::MapperGetSubTreePathsResponse&
                                ifaceList) {
                    if (error)
                    {
                        messages::resourceNotFound(asyncResp->res, "System",
                                                   "hypervisor");
                        return;
                    }
                    asyncResp->res.jsonValue["@odata.type"] =
                        "#EthernetInterfaceCollection."
                        "EthernetInterfaceCollection";
                    asyncResp->res.jsonValue["@odata.id"] =
                        "/redfish/v1/Systems/hypervisor/EthernetInterfaces";
                    asyncResp->res.jsonValue["Name"] = "Hypervisor Ethernet "
                                                       "Interface Collection";
                    asyncResp->res.jsonValue["Description"] =
                        "Collection of Virtual Management "
                        "Interfaces for the hypervisor";

                    nlohmann::json& ifaceArray =
                        asyncResp->res.jsonValue["Members"];
                    ifaceArray = nlohmann::json::array();
                    for (const std::string& iface : ifaceList)
                    {
                        sdbusplus::message::object_path path(iface);
                        std::string name = path.filename();
                        if (name.empty())
                        {
                            continue;
                        }

                        ifaceArray.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Systems/hypervisor/EthernetInterfaces/" +
                                  name}});
                    }
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        ifaceArray.size();
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                "/xyz/openbmc_project/network/hypervisor", 0, interfaces);
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/hypervisor/EthernetInterfaces/<str>/")
        .privileges(redfish::privileges::getEthernetInterface)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& id) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                getHypervisorIfaceData(id, [asyncResp,
                                            ifaceId{std::string(id)}](
                                               const bool& success,
                                               const EthernetInterfaceData&
                                                   ethData,
                                               const boost::container::flat_set<
                                                   IPv4AddressData>& ipv4Data) {
                    if (!success)
                    {
                        messages::resourceNotFound(
                            asyncResp->res, "EthernetInterface", ifaceId);
                        return;
                    }
                    asyncResp->res.jsonValue["@odata.type"] =
                        "#EthernetInterface.v1_5_1.EthernetInterface";
                    asyncResp->res.jsonValue["Name"] =
                        "Hypervisor Ethernet Interface";
                    asyncResp->res.jsonValue["Description"] =
                        "Hypervisor's Virtual Management Ethernet Interface";
                    parseInterfaceData(asyncResp->res.jsonValue, ifaceId,
                                       ethData, ipv4Data);
                });
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/hypervisor/EthernetInterfaces/<str>/")
        .privileges(redfish::privileges::patchEthernetInterface)
        .methods(
            boost::beast::http::verb::
                patch)([&app](
                           const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& ifaceId) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
            {
                return;
            }
            std::optional<std::string> hostName;
            std::optional<std::vector<nlohmann::json>> ipv4StaticAddresses;
            std::optional<nlohmann::json> ipv4Addresses;
            std::optional<nlohmann::json> dhcpv4;
            std::optional<bool> ipv4DHCPEnabled;

            if (!json_util::readJsonPatch(req, asyncResp->res, "HostName",
                                          hostName, "IPv4StaticAddresses",
                                          ipv4StaticAddresses, "IPv4Addresses",
                                          ipv4Addresses, "DHCPv4", dhcpv4))
            {
                return;
            }

            if (ipv4Addresses)
            {
                messages::propertyNotWritable(asyncResp->res, "IPv4Addresses");
                return;
            }

            if (dhcpv4)
            {
                if (!json_util::readJson(*dhcpv4, asyncResp->res, "DHCPEnabled",
                                         ipv4DHCPEnabled))
                {
                    return;
                }
            }

            getHypervisorIfaceData(
                ifaceId,
                [asyncResp, ifaceId, hostName = std::move(hostName),
                 ipv4StaticAddresses = std::move(ipv4StaticAddresses),
                 ipv4DHCPEnabled, dhcpv4 = std::move(dhcpv4)](
                    const bool& success, const EthernetInterfaceData& ethData,
                    const boost::container::flat_set<IPv4AddressData>&) {
                    if (!success)
                    {
                        messages::resourceNotFound(
                            asyncResp->res, "EthernetInterface", ifaceId);
                        return;
                    }

                    if (ipv4StaticAddresses)
                    {
                        const nlohmann::json& ipv4Static = *ipv4StaticAddresses;
                        if (ipv4Static.begin() == ipv4Static.end())
                        {
                            messages::propertyValueTypeError(
                                asyncResp->res,
                                ipv4Static.dump(
                                    2, ' ', true,
                                    nlohmann::json::error_handler_t::replace),
                                "IPv4StaticAddresses");
                            return;
                        }

                        // One and only one hypervisor instance supported
                        if (ipv4Static.size() != 1)
                        {
                            messages::propertyValueFormatError(
                                asyncResp->res,
                                ipv4Static.dump(
                                    2, ' ', true,
                                    nlohmann::json::error_handler_t::replace),
                                "IPv4StaticAddresses");
                            return;
                        }

                        const nlohmann::json& ipv4Json = ipv4Static[0];
                        // Check if the param is 'null'. If its null, it means
                        // that user wants to delete the IP address. Deleting
                        // the IP address is allowed only if its statically
                        // configured. Deleting the address originated from DHCP
                        // is not allowed.
                        if ((ipv4Json.is_null()) &&
                            (translateDHCPEnabledToBool(ethData.DHCPEnabled,
                                                        true)))
                        {
                            BMCWEB_LOG_INFO
                                << "Ignoring the delete on ipv4StaticAddresses "
                                   "as the interface is DHCP enabled";
                        }
                        else
                        {
                            handleHypervisorIPv4StaticPatch(ifaceId, ipv4Static,
                                                            asyncResp);
                        }
                    }

                    if (hostName)
                    {
                        handleHostnamePatch(*hostName, asyncResp);
                    }

                    if (dhcpv4)
                    {
                        setDHCPEnabled(ifaceId, *ipv4DHCPEnabled, asyncResp);
                    }

                    // Set this interface to disabled/inactive. This will be set
                    // to enabled/active by the pldm once the hypervisor
                    // consumes the updated settings from the user.
                    setIPv4InterfaceEnabled(ifaceId, false, asyncResp);
                });
            asyncResp->res.result(boost::beast::http::status::accepted);
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/hypervisor/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                // Only return action info if hypervisor D-Bus object present
                crow::connections::systemBus->async_method_call(
                    [asyncResp](
                        const boost::system::error_code ec,
                        const std::vector<std::pair<
                            std::string, std::vector<std::string>>>& objInfo) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;

                            // No hypervisor objects found by mapper
                            if (ec.value() == boost::system::errc::io_error)
                            {
                                messages::resourceNotFound(asyncResp->res,
                                                           "hypervisor",
                                                           "ResetActionInfo");
                                return;
                            }

                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // One and only one hypervisor instance supported
                        if (objInfo.size() != 1)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // The hypervisor object only support the ability to
                        // turn On The system object Action should be utilized
                        // for other operations
                        asyncResp->res.jsonValue = {
                            {"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                            {"@odata.id",
                             "/redfish/v1/Systems/hypervisor/ResetActionInfo"},
                            {"Name", "Reset Action Info"},
                            {"Id", "ResetActionInfo"},
                            {"Parameters",
                             {{{"Name", "ResetType"},
                               {"Required", true},
                               {"DataType", "String"},
                               {"AllowableValues", {"On"}}}}}};
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject",
                    "/xyz/openbmc_project/state/hypervisor0",
                    std::array<const char*, 1>{
                        "xyz.openbmc_project.State.Host"});
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/hypervisor/Actions/ComputerSystem.Reset/")
        .privileges(redfish::privileges::postComputerSystem)
        .methods(boost::beast::http::verb::post)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp->res))
                {
                    return;
                }
                std::optional<std::string> resetType;
                if (!json_util::readJsonAction(req, asyncResp->res, "ResetType",
                                               resetType))
                {
                    // readJson adds appropriate error to response
                    return;
                }

                if (!resetType)
                {
                    messages::actionParameterMissing(
                        asyncResp->res, "ComputerSystem.Reset", "ResetType");
                    return;
                }

                // Hypervisor object only support On operation
                if (resetType != "On")
                {
                    messages::propertyValueNotInList(asyncResp->res, *resetType,
                                                     "ResetType");
                    return;
                }

                std::string command =
                    "xyz.openbmc_project.State.Host.Transition.On";

                crow::connections::systemBus->async_method_call(
                    [asyncResp, resetType](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                            if (ec.value() ==
                                boost::asio::error::invalid_argument)
                            {
                                messages::actionParameterNotSupported(
                                    asyncResp->res, *resetType, "Reset");
                                return;
                            }

                            if (ec.value() ==
                                boost::asio::error::host_unreachable)
                            {
                                messages::resourceNotFound(asyncResp->res,
                                                           "Actions", "Reset");
                                return;
                            }

                            messages::internalError(asyncResp->res);
                            return;
                        }
                        messages::success(asyncResp->res);
                    },
                    "xyz.openbmc_project.State.Hypervisor",
                    "/xyz/openbmc_project/state/hypervisor0",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.State.Host", "RequestedHostTransition",
                    dbus::utility::DbusVariantType{std::move(command)});
            });
}
} // namespace redfish::hypervisor
