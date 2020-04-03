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
 * Hypervisor Systems derived class for delivering Computer Systems Schema.
 */
class HypervisorSystem : public Node
{
  public:
    /*
     * Default Constructor
     */
    HypervisorSystem(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/hypervisor/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<std::string> &hostName) {
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
                asyncResp->res.jsonValue["Links"]["ManagedBy"] = {
                    {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
                asyncResp->res.jsonValue["EthernetInterfaces"] = {
                    {"@odata.id", "/redfish/v1/Systems/hypervisor/"
                                  "EthernetInterfaces"}};
                // TODO: Add "SystemType" : "hypervisor"
            },
            "xyz.openbmc_project.Settings", "/xyz/openbmc_project/network/vmi",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Network.SystemConfiguration", "HostName");
    }
};

/**
 * HypervisorInterfaceCollection class to handle the GET and PATCH on Hypervisor
 * Interface
 */
class HypervisorInterfaceCollection : public Node
{
  public:
    template <typename CrowApp>
    HypervisorInterfaceCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/hypervisor/EthernetInterfaces/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        const std::array<const char *, 1> interfaces = {
            "xyz.openbmc_project.Network.EthernetInterface"};

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code error_code,
                        const std::vector<std::string> &iface_list) {
                if (error_code)
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
                asyncResp->res.jsonValue["Name"] =
                    "Virtual Management Ethernet "
                    "Network Interface Collection";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of Virtual Management "
                    "Interfaces for the hypervisor";

                nlohmann::json &iface_array =
                    asyncResp->res.jsonValue["Members"];
                iface_array = nlohmann::json::array();
                for (const std::string &iface_item : iface_list)
                {
                    std::size_t last_pos = iface_item.rfind("/");
                    if (last_pos != std::string::npos)
                    {
                        iface_array.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Systems/hypervisor/"
                              "EthernetInterfaces/" +
                                  iface_item.substr(last_pos + 1)}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    iface_array.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/network/vmi", 0, interfaces);
    }
};

inline bool extractHypervisorInterfaceData(const std::string &ethiface_id,
                                           const GetManagedObjects &dbus_data,
                                           EthernetInterfaceData &ethData)
{
    bool idFound = false;
    for (const auto &objpath : dbus_data)
    {
        for (const auto &ifacePair : objpath.second)
        {
            if (objpath.first ==
                "/xyz/openbmc_project/network/vmi/" + ethiface_id)
            {
                idFound = true;
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
                else if (ifacePair.first ==
                         "xyz.openbmc_project.Network.EthernetInterface")
                {
                    for (const auto &propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "DHCPEnabled")
                        {
                            const bool *dhcp =
                                std::get_if<bool>(&propertyPair.second);
                            if (dhcp != nullptr)
                            {
                                ethData.hypervisorDHCPEnabled = *dhcp;
                            }
                        }
                    }
                }
            }
            if (objpath.first == "/xyz/openbmc_project/network/vmi")
            {
                // System configuration shows up in the global namespace, so no
                // need to check eth number
                if (ifacePair.first ==
                    "xyz.openbmc_project.Network.SystemConfiguration")
                {
                    for (const auto &propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "HostName")
                        {
                            const std::string *hostname =
                                sdbusplus::message::variant_ns::get_if<
                                    std::string>(&propertyPair.second);
                            if (hostname != nullptr)
                            {
                                ethData.hostname = *hostname;
                            }
                        }
                        else if (propertyPair.first == "DefaultGateway")
                        {
                            const std::string *defaultGateway =
                                sdbusplus::message::variant_ns::get_if<
                                    std::string>(&propertyPair.second);
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

// Helper function that extracts data for single ethernet ipv4 address
inline void extractHypervisorIPv4Data(
    const std::string &ethiface_id, const GetManagedObjects &dbus_data,
    boost::container::flat_set<IPv4AddressData> &ipv4_config)
{
    const std::string ipv4PathStart =
        "/xyz/openbmc_project/network/vmi/" + ethiface_id + "/ipv4/addr0";

    // Since there might be several IPv4 configurations aligned with
    // single ethernet interface, loop over all of them
    for (const auto &objpath : dbus_data)
    {
        // Check if proper pattern for object path appears
        if (boost::starts_with(objpath.first.str, ipv4PathStart))
        {
            for (auto &interface : objpath.second)
            {
                // Instance IPv4AddressData structure, and set as
                // appropriate
                std::pair<boost::container::flat_set<IPv4AddressData>::iterator,
                          bool>
                    it = ipv4_config.insert(IPv4AddressData{});
                IPv4AddressData &ipv4_address = *it.first;
                ipv4_address.id =
                    objpath.first.str.substr(ipv4PathStart.size());

                if (interface.first == "xyz.openbmc_project.Object.Enable")
                {
                    for (auto &property : interface.second)
                    {
                        if (property.first == "Enabled")
                        {
                            const bool *intfEnable =
                                std::get_if<bool>(&property.second);
                            if (intfEnable != nullptr)
                            {
                                ipv4_address.isActive = *intfEnable;
                            }
                        }
                    }
                }
                if (interface.first == "xyz.openbmc_project.Network.IP")
                {
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
                }
            }
        }
    }
}

/**
 * Function that retrieves all properties for given Hypervisor Ethernet
 * Interface Object from Settings Manager
 * @param ethiface_id a eth interface id to query on DBus
 * @param callback a function that shall be called to convert Dbus output
 * into JSON
 */
template <typename CallbackFunc>
void getHypervisorIfaceData(const std::string &ethiface_id,
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

            bool found =
                extractHypervisorInterfaceData(ethiface_id, resp, ethData);
            if (!found)
            {
                BMCWEB_LOG_DEBUG << "Not found";
                callback(false, ethData, ipv4Data);
                return;
            }
            extractHypervisorIPv4Data(ethiface_id, resp, ipv4Data);
            // Finally make a callback with usefull data
            callback(true, ethData, ipv4Data);
        },
        "xyz.openbmc_project.Settings", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
 * @brief Sets the Hypervisor Interface IPAddress DBUS
 *
 * @param[in] aResp          Shared pointer for generating response message.
 * @param[in] ipv4_address   Address from the incoming request
 * @param[in] ethiface_id    Hypervisor Interface Id
 *
 * @return None.
 */
inline void setHypervisorIPv4Address(std::shared_ptr<AsyncResp> aResp,
                                     const std::string &ethiface_id,
                                     const std::string &ipv4_address)
{
    BMCWEB_LOG_DEBUG << "Setting the Hypervisor IPaddress : " << ipv4_address
                     << " on Iface: " << ethiface_id;
    std::string path =
        "/xyz/openbmc_project/network/vmi/" + ethiface_id + "/ipv4/addr0";
    const char *vmiObj = path.c_str();

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }
            BMCWEB_LOG_DEBUG << "Hypervisor IPaddress is Set";
        },
        "xyz.openbmc_project.Settings", vmiObj,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "Address",
        std::variant<std::string>(ipv4_address));
}

/**
 * @brief Sets the Hypervisor Interface SubnetMask DBUS
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] subnet    SubnetMask from the incoming request
 * @param[in] ethiface_id    Hypervisor Interface Id
 *
 * @return None.
 */
inline void setHypervisorIPv4Subnet(std::shared_ptr<AsyncResp> aResp,
                                    const std::string &ethiface_id,
                                    const uint8_t &subnet)
{
    BMCWEB_LOG_DEBUG << "Setting the Hypervisor subnet : " << subnet
                     << " on Iface: " << ethiface_id;
    std::string path =
        "/xyz/openbmc_project/network/vmi/" + ethiface_id + "/ipv4/addr0";
    const char *vmiObj = path.c_str();

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }
            BMCWEB_LOG_DEBUG << "SubnetMask is Set";
        },
        "xyz.openbmc_project.Settings", vmiObj,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "PrefixLength",
        std::variant<uint8_t>(subnet));
}

/**
 * @brief Sets the Hypervisor Interface Gateway DBUS
 *
 * @param[in] aResp          Shared pointer for generating response message.
 * @param[in] gateway        Gateway from the incoming request
 * @param[in] ethiface_id    Hypervisor Interface Id
 *
 * @return None.
 */
inline void setHypervisorIPv4Gateway(std::shared_ptr<AsyncResp> aResp,
                                     const std::string &ethiface_id,
                                     const std::string &gateway)
{
    BMCWEB_LOG_DEBUG
        << "Setting the DefaultGateway to the last configured gateway";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                return;
            }
            BMCWEB_LOG_DEBUG << "Default Gateway is Set";
        },
        "xyz.openbmc_project.Settings", "/xyz/openbmc_project/network/vmi",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.SystemConfiguration", "DefaultGateway",
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
inline void createHypervisorIPv4(const std::string &ifaceId,
                                 uint8_t prefixLength,
                                 const std::string &gateway,
                                 const std::string &address,
                                 std::shared_ptr<AsyncResp> asyncResp)
{
    setHypervisorIPv4Address(asyncResp, ifaceId, address);
    setHypervisorIPv4Gateway(asyncResp, ifaceId, gateway);
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
inline void deleteHypervisorIPv4(const std::string &ifaceId,
                                 const std::shared_ptr<AsyncResp> asyncResp)
{
    std::string address = "0.0.0.0";
    std::string gateway = "0.0.0.0";
    const uint8_t prefixLength = 0;
    setHypervisorIPv4Address(asyncResp, ifaceId, address);
    setHypervisorIPv4Gateway(asyncResp, ifaceId, gateway);
    setHypervisorIPv4Subnet(asyncResp, ifaceId, prefixLength);
}

/**
 * HypervisorInterface derived class for delivering Ethernet Schema
 */
class HypervisorInterface : public Node
{
  public:
    /*
     * Default Constructor
     */
    template <typename CrowApp>
    HypervisorInterface(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/hypervisor/EthernetInterfaces/<str>/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}}};
    }

  private:
    void parseInterfaceData(
        nlohmann::json &json_response, const std::string &iface_id,
        const EthernetInterfaceData &ethData,
        const boost::container::flat_set<IPv4AddressData> &ipv4Data)
    {
        json_response["Id"] = iface_id;
        json_response["@odata.id"] =
            "/redfish/v1/Systems/hypervisor/EthernetInterfaces/" + iface_id;
        json_response["InterfaceEnabled"] = true;
        json_response["MACAddress"] = ethData.mac_address;

        json_response["HostName"] = ethData.hostname;

        json_response["DHCPv4"]["DHCPEnabled"] = ethData.hypervisorDHCPEnabled;

        nlohmann::json &ipv4_array = json_response["IPv4Addresses"];
        nlohmann::json &ipv4_static_array =
            json_response["IPv4StaticAddresses"];
        ipv4_array = nlohmann::json::array();
        ipv4_static_array = nlohmann::json::array();
        for (auto &ipv4_config : ipv4Data)
        {
            if (ipv4_config.isActive)
            {
                // Virtual management Inteface has applied the IP settings.
                // Thus display the active IP details to the user
                ipv4_array.push_back({{"AddressOrigin", ipv4_config.origin},
                                      {"SubnetMask", ipv4_config.netmask},
                                      {"Address", ipv4_config.address},
                                      {"Gateway", ethData.default_gateway}});
                if (ipv4_config.origin == "Static")
                {
                    ipv4_static_array.push_back(
                        {{"AddressOrigin", ipv4_config.origin},
                         {"SubnetMask", ipv4_config.netmask},
                         {"Address", ipv4_config.address},
                         {"Gateway", ethData.default_gateway}});
                }
            }
            else
            {
                // The virtual management interface has not applied the settings
                // Dont show the IP details.
                ipv4_array.push_back({{"AddressOrigin", ipv4_config.origin},
                                      {"SubnetMask", "0.0.0.0"},
                                      {"Address", "0.0.0.0"},
                                      {"Gateway", "0.0.0.0"}});
                if (ipv4_config.origin == "Static")
                {
                    ipv4_static_array.push_back(
                        {{"AddressOrigin", ipv4_config.origin},
                         {"SubnetMask", "0.0.0.0"},
                         {"Address", "0.0.0.0"},
                         {"Gateway", "0.0.0.0"}});
                }
            }
        }
    }

    void handleHypervisorIPv4StaticPatch(
        const std::string &ifaceId, nlohmann::json &input,
        const std::shared_ptr<AsyncResp> asyncResp)
    {
        if ((!input.is_array()) || input.empty())
        {
            messages::propertyValueTypeError(asyncResp->res, input.dump(),
                                             "IPv4StaticAddresses");
            return;
        }

        unsigned entryIdx = 1;

        for (nlohmann::json &thisJson : input)
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

                uint8_t prefixLength = 0;
                bool errorInEntry = false;
                if (address)
                {
                    if (!ipv4VerifyIpAndGetBitcount(*address))
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, *address, pathString + "/Address");
                        errorInEntry = true;
                    }
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
                        messages::propertyValueFormatError(
                            asyncResp->res, *gateway, pathString + "/Gateway");
                        errorInEntry = true;
                    }
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

                BMCWEB_LOG_DEBUG
                    << "Calling createHypervisorIPv4 on : " << ifaceId << ","
                    << *address;
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
            // Set this interface to inActive
            const bool isActive = false;
            setIPv4IntfStatus(ifaceId, isActive, asyncResp);

            break; // Hypervisor considers the first IP address in the array
                   // list.Thus break here.
        }
    }

    bool isHostnameValid(const std::string &hostname)
    {
        // A valid host name can never have the dotted-decimal form
        if (std::all_of(hostname.begin(), hostname.end(), ::isdigit))
        {
            return false;
        }
        // Allow up to 255 characters
        if (hostname.length() > 255)
        {
            return false;
        }
        // Validate the regex
        const std::regex pattern(
            "^[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]$");

        return std::regex_match(hostname, pattern);
    }

    void handleHostnamePatch(const std::string &hostname,
                             const std::shared_ptr<AsyncResp> asyncResp)
    {
        if (!isHostnameValid(hostname))
        {
            messages::propertyValueFormatError(asyncResp->res, hostname,
                                               "HostName");
            return;
        }

        asyncResp->res.jsonValue["HostName"] = hostname;
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                }
            },
            "xyz.openbmc_project.Settings", "/xyz/openbmc_project/network/vmi",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
            std::variant<std::string>(hostname));
    }

    void setIPv4IntfStatus(const std::string &ifaceId, const bool &isActive,
                           const std::shared_ptr<AsyncResp> asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                    return;
                }
            },
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/network/vmi/" + ifaceId + "ipv4/addr0",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Object.Enable", "Enabled",
            std::variant<bool>(isActive));
    }

    void setDHCPEnabled(const std::string &ifaceId, const bool &v4Value,
                        const std::shared_ptr<AsyncResp> asyncResp)
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
            "/xyz/openbmc_project/network/vmi/" + ifaceId,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.EthernetInterface", "DHCPEnabled",
            std::variant<bool>(v4Value));

        // Set this interface to inActive
        const bool isActive = false;
        setIPv4IntfStatus(ifaceId, isActive, asyncResp);

        // If DHCPEnabled is set to true. Delete the current IPv4 settings
        // to receive the new values from DHCP server.
        if (v4Value == true)
        {
            deleteHypervisorIPv4(ifaceId, asyncResp);
        }
        // Set the IPv4 address origin to the DHCP / Static as per the new value
        // of the DHCPEnabled property
        std::string path =
            "/xyz/openbmc_project/network/vmi/" + ifaceId + "/ipv4/addr0";
        const char *vmiObj = path.c_str();
        std::string origin;
        if (v4Value == false)
        {
            origin = "xyz.openbmc_project.Network.IP.AddressOrigin.Static";
        }
        else
        {
            origin = "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP";
        }
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                    return;
                }
                BMCWEB_LOG_DEBUG << "Hypervisor IPaddress Origin is Set";
            },
            "xyz.openbmc_project.Settings", vmiObj,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.IP", "Origin",
            std::variant<std::string>(origin));
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

        getHypervisorIfaceData(
            params[0],
            [this, asyncResp, iface_id{std::string(params[0])}](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (!success)
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "EthernetInterface", iface_id);
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#EthernetInterface.v1_5_1.EthernetInterface";
                asyncResp->res.jsonValue["Name"] =
                    "Virtual Management Ethernet Interface";
                asyncResp->res.jsonValue["Description"] =
                    "Virtual Interface Management Network Interface";
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

        std::optional<std::string> hostname;
        std::optional<nlohmann::json> ipv4StaticAddresses;
        std::optional<nlohmann::json> dhcpv4;
        std::optional<bool> v4Value;

        if (!json_util::readJson(req, res, "HostName", hostname,
                                 "IPv4StaticAddresses", ipv4StaticAddresses,
                                 "DHCPv4", dhcpv4))
        {
            return;
        }

        if (dhcpv4)
        {
            if (!json_util::readJson(*dhcpv4, res, "DHCPEnabled", v4Value))
            {
                return;
            }
        }

        getHypervisorIfaceData(
            iface_id,
            [this, asyncResp, iface_id, hostname = std::move(hostname),
             ipv4StaticAddresses = std::move(ipv4StaticAddresses),
             v4Value = std::move(v4Value), dhcpv4 = std::move(dhcpv4)](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (!success)
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "EthernetInterface", iface_id);
                    return;
                }
                if (ipv4StaticAddresses)
                {
                    nlohmann::json ipv4Static = std::move(*ipv4StaticAddresses);
                    handleHypervisorIPv4StaticPatch(iface_id, ipv4Static,
                                                    asyncResp);
                }

                if (hostname)
                {
                    handleHostnamePatch(*hostname, asyncResp);
                }

                if (dhcpv4)
                {
                    setDHCPEnabled(iface_id, *v4Value, asyncResp);
                }
            });

        // TODO : Task will be created for monitoring the Hypervisor interface
        // Hypervisor will notify once the IP is applied to the Hypervisor.
        // The status will be sent over to the client.
        res.result(boost::beast::http::status::accepted);
    }
};
} // namespace redfish
