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
            [asyncResp](const boost::system::error_code error,
                        const std::vector<std::string> &ifaceList) {
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
                asyncResp->res.jsonValue["Name"] =
                    "Virtual Management Ethernet "
                    "Network Interface Collection";
                asyncResp->res.jsonValue["Description"] =
                    "Collection of Virtual Management "
                    "Interfaces for the hypervisor";

                nlohmann::json &ifaceArray =
                    asyncResp->res.jsonValue["Members"];
                ifaceArray = nlohmann::json::array();
                for (const std::string &iface : ifaceList)
                {
                    std::size_t last_pos = iface.rfind("/");
                    if (last_pos != std::string::npos)
                    {
                        ifaceArray.push_back(
                            {{"@odata.id", "/redfish/v1/Systems/hypervisor/"
                                           "EthernetInterfaces/" +
                                               iface.substr(last_pos + 1)}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    ifaceArray.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/network/vmi", 0, interfaces);
    }
};

inline bool extractHypervisorInterfaceData(
    const std::string &ethifaceId, const GetManagedObjects &dbusData,
    EthernetInterfaceData &ethData,
    boost::container::flat_set<IPv4AddressData> &ipv4Config)
{
    bool idFound = false;
    for (const auto &objpath : dbusData)
    {
        for (const auto &ifacePair : objpath.second)
        {
            if (objpath.first ==
                "/xyz/openbmc_project/network/vmi/" + ethifaceId)
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
            }
            if (objpath.first == "/xyz/openbmc_project/network/vmi/" +
                                     ethifaceId + "/ipv4/addr0")
            {
                if (ifacePair.first == "xyz.openbmc_project.Network.IP")
                {
                    // Instance IPv4AddressData structure, and set as
                    // appropriate
                    const std::string ipv4PathStart =
                        "/xyz/openbmc_project/network/vmi/" + ethifaceId +
                        "/ipv4/addr0";
                    std::pair<
                        boost::container::flat_set<IPv4AddressData>::iterator,
                        bool>
                        it = ipv4Config.insert(IPv4AddressData{});
                    IPv4AddressData &ipv4Address = *it.first;
                    ipv4Address.id =
                        objpath.first.str.substr(ipv4PathStart.size());
                    for (auto &property : ifacePair.second)
                    {
                        if (property.first == "Address")
                        {
                            const std::string *address =
                                std::get_if<std::string>(&property.second);
                            if (address != nullptr)
                            {
                                ipv4Address.address = *address;
                            }
                        }
                        else if (property.first == "Origin")
                        {
                            const std::string *origin =
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
                            const uint8_t *mask =
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
/**
 * Function that retrieves all properties for given VMI Ethernet Interface
 * Object from Settings Manager
 * @param ethifaceId a eth interface id to query on DBus
 * @param callback a function that shall be called to convert Dbus output
 * into JSON
 */
template <typename CallbackFunc>
void getHypervisorIfaceData(const std::string &ethifaceId,
                            CallbackFunc &&callback)
{
    crow::connections::systemBus->async_method_call(
        [ethifaceId{std::string{ethifaceId}},
         callback{std::move(callback)}](const boost::system::error_code error,
                                        const GetManagedObjects &resp) {
            EthernetInterfaceData ethData{};
            boost::container::flat_set<IPv4AddressData> ipv4Data;
            if (error)
            {
                callback(false, ethData, ipv4Data);
                return;
            }

            bool found = extractHypervisorInterfaceData(ethifaceId, resp,
                                                        ethData, ipv4Data);
            if (!found)
            {
                BMCWEB_LOG_DEBUG << "Not found";
                callback(false, ethData, ipv4Data);
                return;
            }
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
 * @param[in] ipv4Aaddress   Address from the incoming request
 * @param[in] ethifaceId     Hypervisor Interface Id
 *
 * @return None.
 */
inline void setHypervisorIPv4Address(std::shared_ptr<AsyncResp> aResp,
                                     const std::string &ethifaceId,
                                     const std::string &ipv4Aaddress)
{
    BMCWEB_LOG_DEBUG << "Setting the Hypervisor IPaddress : " << ipv4Aaddress
                     << " on Iface: " << ethifaceId;
    std::string path =
        "/xyz/openbmc_project/network/vmi/" + ethifaceId + "/ipv4/addr0";
    const char *vmiObj = path.c_str();

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // not an error, don't have to have the interface
                return;
            }
            BMCWEB_LOG_DEBUG << "Hypervisor IPaddress is Set";
        },
        "xyz.openbmc_project.Settings", vmiObj,
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Network.IP", "Address",
        std::variant<std::string>(ipv4Aaddress));
}

/**
 * @brief Sets the Hypervisor Interface SubnetMask DBUS
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] subnet    SubnetMask from the incoming request
 * @param[in] ethifaceId  Hypervisor Interface Id
 *
 * @return None.
 */
inline void setHypervisorIPv4Subnet(std::shared_ptr<AsyncResp> aResp,
                                    const std::string &ethifaceId,
                                    const uint8_t &subnet)
{
    BMCWEB_LOG_DEBUG << "Setting the Hypervisor subnet : " << subnet
                     << " on Iface: " << ethifaceId;
    std::string path =
        "/xyz/openbmc_project/network/vmi/" + ethifaceId + "/ipv4/addr0";
    const char *vmiObj = path.c_str();

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // not an error, don't have to have the interface
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
 * @param[in] ethifaceId     Hypervisor Interface Id
 *
 * @return None.
 */
inline void setHypervisorIPv4Gateway(std::shared_ptr<AsyncResp> aResp,
                                     const std::string &ethifaceId,
                                     const std::string &gateway)
{
    BMCWEB_LOG_DEBUG
        << "Setting the DefaultGateway to the last configured gateway";

    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                // not an error, don't have to have the interface
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
        nlohmann::json &jsonResponse, const std::string &ifaceId,
        const EthernetInterfaceData &ethData,
        const boost::container::flat_set<IPv4AddressData> &ipv4Data)
    {
        jsonResponse["Id"] = ifaceId;
        jsonResponse["@odata.id"] =
            "/redfish/v1/Systems/hypervisor/EthernetInterfaces/" + ifaceId;
        jsonResponse["InterfaceEnabled"] = true;
        jsonResponse["MACAddress"] = ethData.mac_address;

        if (!ethData.hostname.empty())
        {
            jsonResponse["HostName"] = ethData.hostname;
        }

        nlohmann::json &ipv4Array = jsonResponse["IPv4Addresses"];
        nlohmann::json &ipv4StaticArray = jsonResponse["IPv4StaticAddresses"];
        ipv4Array = nlohmann::json::array();
        ipv4StaticArray = nlohmann::json::array();
        for (auto &ipv4Config : ipv4Data)
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
            }
            else
            {
                if (thisJson.is_null())
                {
                    deleteHypervisorIPv4(ifaceId, asyncResp);
                }
            }
            break; // Hypervisor considers the first IP address in the array
                   // list.Thus break here.
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

        getHypervisorIfaceData(
            params[0],
            [this, asyncResp, ifaceId{std::string(params[0])}](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (!success)
                {
                    messages::resourceNotFound(
                        asyncResp->res, "HostEthernetInterface", ifaceId);
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#EthernetInterface.v1_5_1.EthernetInterface";
                asyncResp->res.jsonValue["Name"] =
                    "Virtual Management Ethernet Interface";
                asyncResp->res.jsonValue["Description"] =
                    "Virtual Interface Management Network Interface";
                parseInterfaceData(asyncResp->res.jsonValue, ifaceId, ethData,
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

        std::optional<nlohmann::json> ipv4StaticAddresses;

        if (!json_util::readJson(req, res, "IPv4StaticAddresses",
                                 ipv4StaticAddresses))
        {
            return;
        }

        if (ipv4StaticAddresses)
        {
            nlohmann::json ipv4Static = std::move(*ipv4StaticAddresses);
            handleHypervisorIPv4StaticPatch(iface_id, ipv4Static, asyncResp);
        }

        // TODO : Task will be created for monitoring the hypervisor interface
        // Hypervisor will notify once the IP is applied to the hypervisor.
        // The status will be sent over to the client.
        res.result(boost::beast::http::status::accepted);
    }
};
} // namespace redfish
