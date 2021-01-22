#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <error_messages.hpp>
#include <node.hpp>
#include <utils/json_utils.hpp>

#include <optional>
#include <utility>
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
    HypervisorSystem(App& app) : Node(app, "/redfish/v1/Systems/hypervisor/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<std::string>& /*hostName*/) {
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
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/network/hypervisor",
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
    HypervisorInterfaceCollection(App& app) :
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        const std::array<const char*, 1> interfaces = {
            "xyz.openbmc_project.Network.EthernetInterface"};

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code error,
                        const std::vector<std::string>& ifaceList) {
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
                    std::size_t lastPos = iface.rfind('/');
                    if (lastPos != std::string::npos)
                    {
                        ifaceArray.push_back(
                            {{"@odata.id", "/redfish/v1/Systems/hypervisor/"
                                           "EthernetInterfaces/" +
                                               iface.substr(lastPos + 1)}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    ifaceArray.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/network/hypervisor", 0, interfaces);
    }
};

inline std::string getIfAttributeName(const std::string& ifaceId)
{

    if (ifaceId == "eth0")
    {
        return "if0";
    }
    else if (ifaceId == "eth1")
    {
        return "if1";
    }
    return "if0";
}

inline bool extractHypervisorInterfaceData(
    const std::string& ethIfaceId, const GetManagedObjects& dbusData,
    EthernetInterfaceData& ethData,
    boost::container::flat_set<IPv4AddressData>& ipv4Config,
    std::shared_ptr<std::map<
        std::string,
        std::tuple<std::string, bool, std::string, std::string, std::string,
                   std::variant<int64_t, std::string>,
                   std::variant<int64_t, std::string>,
                   std::vector<std::tuple<
                       std::string, std::variant<int64_t, std::string>>>>>>
        basebiosTable)
{
    bool idFound = false;
    for (const auto& objpath : dbusData)
    {
        for (const auto& ifacePair : objpath.second)
        {
            if (objpath.first == "/xyz/openbmc_project/network/hypervisor/" +
                                     ethIfaceId + "/ipv4/addr0")
            {
                idFound = true;
                std::pair<boost::container::flat_set<IPv4AddressData>::iterator,
                          bool>
                    it = ipv4Config.insert(IPv4AddressData{});
                IPv4AddressData& ipv4Address = *it.first;
                if (ifacePair.first == "xyz.openbmc_project.Object.Enable")
                {
                    for (auto& property : ifacePair.second)
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

                for (const auto& key : *basebiosTable)
                {
                    if (key.first == "vmi_" + getIfAttributeName(ethIfaceId) +
                                         "_ipv4_ipaddr")
                    {
                        const std::string* address = std::get_if<std::string>(
                            &(std::get<5>(key.second)));
                        if (address != nullptr)
                        {
                            ipv4Address.address = *address;
                            BMCWEB_LOG_DEBUG << key.first
                                             << ipv4Address.address;
                        }
                    }
                    if (key.first == "vmi_" + getIfAttributeName(ethIfaceId) +
                                         "_ipv4_prefix_length")
                    {
                        // Attibutes values can only be int64_t or string as per
                        // the bios-settings-mgr dbus interfaces.
                        // So we need to convert an int64_t to uint32_t to
                        // leverage the helper function getNetmask() to convert
                        // the prefixlength to subnetmask.

                        int64_t prefixlength =
                            std::get<int64_t>(std::get<5>(key.second));
                        uint32_t* p =
                            reinterpret_cast<uint32_t*>(&prefixlength);
                        ipv4Address.netmask = getNetmask(*p);
                        BMCWEB_LOG_DEBUG
                            << key.first
                            << std::get<int64_t>(std::get<5>(key.second));
                    }
                    if (key.first == "vmi_" + getIfAttributeName(ethIfaceId) +
                                         "_ipv4_gateway")
                    {
                        const std::string* gateway = std::get_if<std::string>(
                            &(std::get<5>(key.second)));
                        if (gateway != nullptr)
                        {
                            ipv4Address.gateway = *gateway;
                            BMCWEB_LOG_DEBUG << key.first
                                             << ipv4Address.gateway;
                        }
                    }

                    if (key.first == "vmi_" + getIfAttributeName(ethIfaceId) +
                                         "_ipv4_method")
                    {
                        const std::string* origin = std::get_if<std::string>(
                            &(std::get<5>(key.second)));
                        if (origin != nullptr)
                        {
                            if (*origin == "IPv4Static")
                            {
                                ipv4Address.origin = "Static";
                                ethData.DHCPEnabled =
                                    "xyz.openbmc_project.Network."
                                    "EthernetInterface.DHCPConf.none";
                                BMCWEB_LOG_DEBUG << key.first
                                                 << ipv4Address.origin;
                            }
                            else if (*origin == "IPv4DHCP")
                            {
                                ipv4Address.origin = "DHCP";
                                ethData.DHCPEnabled =
                                    "xyz.openbmc_project.Network."
                                    "EthernetInterface.DHCPConf.v4";
                                BMCWEB_LOG_DEBUG << key.first
                                                 << ipv4Address.origin;
                            }
                            else
                            {
                                // hypervisor did not set the Origin so setting
                                // it as static by default, as an enum value
                                // cannot but NULL
                                ipv4Address.origin = "Static";
                                ethData.DHCPEnabled =
                                    "xyz.openbmc_project.Network."
                                    "EthernetInterface.DHCPConf.none";
                                BMCWEB_LOG_DEBUG
                                    << "Setting the Origin as Static as "
                                       "hypervisor gateway attribute is not "
                                       "set";
                            }
                        }
                    }
                    if (key.first == "vmi_hostname")
                    {
                        const std::string* hostName = std::get_if<std::string>(
                            &(std::get<5>(key.second)));
                        if (hostName != nullptr)
                        {
                            ethData.hostname = *hostName;
                            BMCWEB_LOG_DEBUG << key.first << ethData.hostname;
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
    using basebiostablevariant_t = std::map<
        std::string,
        std::tuple<std::string, bool, std::string, std::string, std::string,
                   std::variant<int64_t, std::string>,
                   std::variant<int64_t, std::string>,
                   std::vector<std::tuple<
                       std::string, std::variant<int64_t, std::string>>>>>;

    auto basebiosTable = std::make_shared<basebiostablevariant_t>();

    crow::connections::systemBus->async_method_call(
        [basebiosTable, ethIfaceId{std::string{ethIfaceId}},
         callback{std::move(callback)}](
            const boost::system::error_code ec,
            const std::variant<basebiostablevariant_t>&
                returnbiostableMessage) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec.value()
                                 << ec.category().name();
                return;
            }
            *basebiosTable =
                std::get<basebiostablevariant_t>(returnbiostableMessage);

            if (basebiosTable == nullptr)
            {
                BMCWEB_LOG_ERROR << "baseBiosTable == nullptr ";
                return;
            }

            crow::connections::systemBus->async_method_call(
                [ethIfaceId{std::string{ethIfaceId}},
                 callback{std::move(callback)},
                 basebiosTable](const boost::system::error_code error,
                                const GetManagedObjects& resp) {
                    EthernetInterfaceData ethData{};
                    boost::container::flat_set<IPv4AddressData> ipv4Data;
                    if (error)
                    {
                        callback(false, ethData, ipv4Data);
                        return;
                    }

                    bool found = extractHypervisorInterfaceData(
                        ethIfaceId, resp, ethData, ipv4Data, basebiosTable);

                    if (!found)
                    {
                        BMCWEB_LOG_DEBUG << "Hypervisor Interface not found";
                    }
                    callback(found, ethData, ipv4Data);
                },
                "xyz.openbmc_project.Settings", "/",
                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        },
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");
}

inline void setVmiBiosEthernetInterfaceAttribute(
    const std::map<std::string,
                   std::tuple<std::string, std::variant<int64_t, std::string>>>&
        newPendingAttributes,
    const std::shared_ptr<AsyncResp> asyncResp)
{
    // Get the pending attribute table and append the new attributes
    // so that we dont loose the attributes that are set by other apps

    using pendingAttributes_t =
        std::map<std::string,
                 std::tuple<std::string, std::variant<int64_t, std::string>>>;
    crow::connections::systemBus->async_method_call(
        [asyncResp, newPendingAttributes](
            const boost::system::error_code ec,
            std::variant<pendingAttributes_t> retpendingAttributes) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
            }
            pendingAttributes_t pendingAttributes =
                std::get<pendingAttributes_t>(retpendingAttributes);

            // Check the pending attributes to see if we want to add a new
            // attribute or update an existing attribute

            BMCWEB_LOG_DEBUG << pendingAttributes.size();

            for (const auto& newattributeKey : newPendingAttributes)
            {
                pendingAttributes.insert_or_assign(newattributeKey.first,
                                                   newattributeKey.second);
            }

            BMCWEB_LOG_DEBUG << "After adding new" << pendingAttributes.size();

            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                    }
                },
                "xyz.openbmc_project.BIOSConfigManager",
                "/xyz/openbmc_project/bios_config/manager",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.BIOSConfig.Manager", "PendingAttributes",
                std::variant<pendingAttributes_t>(pendingAttributes));
        },
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.BIOSConfig.Manager", "PendingAttributes");
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
inline void createHypervisorIPv4(const std::string& ifaceId,
                                 const int64_t prefixLength,
                                 const std::string& gateway,
                                 const std::string& address,
                                 const bool& dhcpEnabled,
                                 const std::shared_ptr<AsyncResp> asyncResp)
{
    // create a map with ipaddress, gateway & prefixlength
    using pendingAttributes_t =
        std::map<std::string,
                 std::tuple<std::string, std::variant<int64_t, std::string>>>;
    pendingAttributes_t pendingAttributes;
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_ipaddr",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
            address));
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_gateway",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
            gateway));
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_prefix_length",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer",
            prefixLength));
    std::string origin;
    if (dhcpEnabled == false)
    {
        origin = "IPv4Static";
    }
    else
    {
        origin = "IPv4DHCP";
    }
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_method",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
            origin));
    setVmiBiosEthernetInterfaceAttribute(pendingAttributes, asyncResp);
}

/**
 * @brief Deletes given IPv4 interface
 *
 * @param[in] ifaceId     Id of interface whose IP should be deleted
 * @param[io] asyncResp   Response object that will be returned to client
 *
 * @return None
 */
inline void deleteHypervisorIPv4(const std::string& ifaceId,
                                 const bool& dhcpEnabled,
                                 const std::shared_ptr<AsyncResp>& asyncResp)
{
    std::string address = "0.0.0.0";
    std::string gateway = "0.0.0.0";
    const int64_t prefixLength = 0;

    // create a map with ipaddress, gateway & prefixlength
    using pendingAttributes_t =
        std::map<std::string,
                 std::tuple<std::string, std::variant<int64_t, std::string>>>;
    pendingAttributes_t pendingAttributes;
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_ipaddr",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
            address));
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_gateway",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
            gateway));
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_prefix_length",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer",
            prefixLength));
    std::string origin;
    if (dhcpEnabled == false)
    {
        origin = "IPv4Static";
    }
    else
    {
        origin = "IPv4DHCP";
    }
    pendingAttributes.emplace(
        "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_method",
        std::make_tuple(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
            origin));
    setVmiBiosEthernetInterfaceAttribute(pendingAttributes, asyncResp);
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
    HypervisorInterface(App& app) :
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
        nlohmann::json& jsonResponse, const std::string& ifaceId,
        const EthernetInterfaceData& ethData,
        const boost::container::flat_set<IPv4AddressData>& ipv4Data)
    {
        jsonResponse["Id"] = ifaceId;
        jsonResponse["@odata.id"] =
            "/redfish/v1/Systems/hypervisor/EthernetInterfaces/" + ifaceId;
        jsonResponse["InterfaceEnabled"] = true;

        jsonResponse["HostName"] = ethData.hostname;
        jsonResponse["DHCPv4"]["DHCPEnabled"] =
            translateDHCPEnabledToBool(ethData.DHCPEnabled, true);

        nlohmann::json& ipv4Array = jsonResponse["IPv4Addresses"];
        nlohmann::json& ipv4StaticArray = jsonResponse["IPv4StaticAddresses"];
        ipv4Array = nlohmann::json::array();
        ipv4StaticArray = nlohmann::json::array();
        for (auto& ipv4Config : ipv4Data)
        {
            if (ipv4Config.isActive)
            {

                ipv4Array.push_back({{"AddressOrigin", ipv4Config.origin},
                                     {"SubnetMask", ipv4Config.netmask},
                                     {"Address", ipv4Config.address},
                                     {"Gateway", ipv4Config.gateway}});
                if (ipv4Config.origin == "Static")
                {
                    ipv4StaticArray.push_back(
                        {{"AddressOrigin", ipv4Config.origin},
                         {"SubnetMask", ipv4Config.netmask},
                         {"Address", ipv4Config.address},
                         {"Gateway", ipv4Config.gateway}});
                }
            }
        }
    }

    void handleHypervisorIPv4StaticPatch(
        const std::string& ifaceId, const nlohmann::json& input,
        const std::shared_ptr<AsyncResp>& asyncResp)
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
                                     address, "SubnetMask", subnetMask,
                                     "Gateway", gateway))
            {
                messages::propertyValueFormatError(asyncResp->res,
                                                   thisJson.dump(), pathString);
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
                    messages::propertyValueFormatError(asyncResp->res, *gateway,
                                                       pathString + "/Gateway");
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

            BMCWEB_LOG_DEBUG << "Calling createHypervisorIPv4 on : " << ifaceId
                             << "," << *address;
            createHypervisorIPv4(ifaceId, prefixLength, *gateway, *address,
                                 false, asyncResp);
        }
        else
        {
            if (thisJson.is_null())
            {
                deleteHypervisorIPv4(ifaceId, false, asyncResp);
            }
        }
    }

    bool isHostnameValid(const std::string& hostName)
    {
        // As per RFC 1123
        // Allow up to 255 characters
        if (hostName.length() > 255)
        {
            return false;
        }
        // Validate the regex
        const std::regex pattern(
            "^[a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9]$");

        return std::regex_match(hostName, pattern);
    }

    inline void setVmiBiosHostNameAttribute(
        std::map<std::string,
                 std::tuple<std::string, std::variant<int64_t, std::string>>>&
            newPendingAttributes,
        const std::shared_ptr<AsyncResp>& asyncResp)
    {
        using pendingAttributes_t = std::map<
            std::string,
            std::tuple<std::string, std::variant<int64_t, std::string>>>;

        crow::connections::systemBus->async_method_call(
            [asyncResp, newPendingAttributes](
                const boost::system::error_code ec,
                std::variant<pendingAttributes_t> retpendingAttributes) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                }

                pendingAttributes_t pendingAttributes =
                    std::get<pendingAttributes_t>(retpendingAttributes);

                for (const auto& newattributeKey : newPendingAttributes)
                {
                    pendingAttributes.insert_or_assign(newattributeKey.first,
                                                       newattributeKey.second);
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                        }
                    },
                    "xyz.openbmc_project.BIOSConfigManager",
                    "/xyz/openbmc_project/bios_config/manager",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.BIOSConfig.Manager",
                    "PendingAttributes",
                    std::variant<pendingAttributes_t>(pendingAttributes));
            },
            "xyz.openbmc_project.BIOSConfigManager",
            "/xyz/openbmc_project/bios_config/manager",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.BIOSConfig.Manager", "PendingAttributes");
    }

    void setIPv4InterfaceEnabled(const std::string& ifaceId,
                                 const bool& isActive,
                                 const std::shared_ptr<AsyncResp>& asyncResp)
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
            "/xyz/openbmc_project/network/hypervisor/" + ifaceId +
                "/ipv4/addr0",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Object.Enable", "Enabled",
            std::variant<bool>(isActive));
    }

    void setDHCPEnabled(const std::string& ifaceId, const bool& ipv4DHCPEnabled,
                        const std::shared_ptr<AsyncResp>& asyncResp)
    {

        using pendingAttributes_t = std::map<
            std::string,
            std::tuple<std::string, std::variant<int64_t, std::string>>>;
        pendingAttributes_t pendingAttributes;
        if (ipv4DHCPEnabled == true)
        {
            deleteHypervisorIPv4(ifaceId, ipv4DHCPEnabled, asyncResp);
        }
        else
        {
            pendingAttributes.emplace(
                "vmi_" + getIfAttributeName(ifaceId) + "_ipv4_method",
                std::make_tuple("xyz.openbmc_project.BIOSConfig.Manager."
                                "AttributeType.String",
                                "IPv4Static"));
            setVmiBiosEthernetInterfaceAttribute(pendingAttributes, asyncResp);
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

        getHypervisorIfaceData(
            params[0],
            [this, asyncResp, ifaceId{std::string(params[0])}](
                const bool& success, const EthernetInterfaceData& ethData,
                const boost::container::flat_set<IPv4AddressData>& ipv4Data) {
                if (!success)
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "EthernetInterface", ifaceId);
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#EthernetInterface.v1_5_1.EthernetInterface";
                asyncResp->res.jsonValue["Name"] =
                    "Hypervisor Ethernet Interface";
                asyncResp->res.jsonValue["Description"] =
                    "Hypervisor's Virtual Management Ethernet Interface";
                parseInterfaceData(asyncResp->res.jsonValue, ifaceId, ethData,
                                   ipv4Data);
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
        std::optional<std::string> hostName;
        std::optional<nlohmann::json> ipv4StaticAddresses;
        std::optional<nlohmann::json> ipv4Addresses;
        std::optional<nlohmann::json> dhcpv4;
        std::optional<bool> ipv4DHCPEnabled;

        if (!json_util::readJson(req, res, "HostName", hostName,
                                 "IPv4StaticAddresses", ipv4StaticAddresses,
                                 "IPv4Addresses", ipv4Addresses, "DHCPv4",
                                 dhcpv4))
        {
            return;
        }

        if (ipv4Addresses)
        {
            messages::propertyNotWritable(asyncResp->res, "IPv4Addresses");
        }

        if (dhcpv4)
        {
            if (!json_util::readJson(*dhcpv4, res, "DHCPEnabled",
                                     ipv4DHCPEnabled))
            {
                return;
            }
        }

        getHypervisorIfaceData(
            ifaceId,
            [this, asyncResp, ifaceId, hostName = std::move(hostName),
             ipv4StaticAddresses = std::move(ipv4StaticAddresses),
             ipv4DHCPEnabled, dhcpv4 = std::move(dhcpv4)](
                const bool& success, const EthernetInterfaceData& ethData,
                const boost::container::flat_set<IPv4AddressData>&) {
                if (!success)
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "EthernetInterface", ifaceId);
                    return;
                }

                if (ipv4StaticAddresses)
                {
                    const nlohmann::json& ipv4Static = *ipv4StaticAddresses;
                    if (ipv4Static.begin() == ipv4Static.end())
                    {
                        messages::propertyValueTypeError(asyncResp->res,
                                                         ipv4Static.dump(),
                                                         "IPv4StaticAddresses");
                        return;
                    }

                    const nlohmann::json& ipv4Json = ipv4Static[0];
                    // Check if the param is 'null'. If its null, it means that
                    // user wants to delete the IP address. Deleting the IP
                    // address is allowed only if its statically configured.
                    // Deleting the address originated from DHCP is not allowed.
                    if ((ipv4Json.is_null()) &&
                        (translateDHCPEnabledToBool(ethData.DHCPEnabled, true)))
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
                    if (!isHostnameValid(*hostName))
                    {
                        messages::propertyValueFormatError(
                            asyncResp->res, *hostName, "HostName");
                        return;
                    }

                    using pendingAttributes_t = std::map<
                        std::string,
                        std::tuple<std::string,
                                   std::variant<int64_t, std::string>>>;
                    pendingAttributes_t pendingAttributes;
                    pendingAttributes.emplace(
                        "vmi_hostname",
                        std::make_tuple("xyz.openbmc_project.BIOSConfig."
                                        "Manager.AttributeType.String",
                                        *hostName));
                    setVmiBiosHostNameAttribute(pendingAttributes, asyncResp);
                }

                if (dhcpv4)
                {
                    setDHCPEnabled(ifaceId, *ipv4DHCPEnabled, asyncResp);
                }

                // Set this interface to disabled/inactive. This will be set to
                // enabled/active by the pldm once the hypervisor consumes the
                // updated settings from the user.
                setIPv4InterfaceEnabled(ifaceId, false, asyncResp);
            });
        res.result(boost::beast::http::status::accepted);
    }
};
} // namespace redfish
