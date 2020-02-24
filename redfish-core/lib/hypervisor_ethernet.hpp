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
 *  * Hypervisor Systems derived class for delivering Computer Systems Schema.
 *   */
class HypervisorSystem : public Node
{
  public:
    /*
     *      * Default Constructor
     *           */
    HypervisorSystem(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/hypervisor/")
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
     *      * Functions triggers appropriate requests on DBus
     *           */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] = "#ComputerSystem.v1_6_0.ComputerSystem";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/hypervisor";
        res.jsonValue["Description"] = "Power Hypervisor";
        res.jsonValue["Name"] = "Power Hypervisor";
        res.jsonValue["Id"] = "hypervisor";
        res.jsonValue["Links"]["ManagedBy"] = {
            {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
        res.jsonValue["EthernetInterfaces"] = {
            {"@odata.id", "/redfish/v1/Systems/hypervisor/EthernetInterfaces"}};
        res.end();
    }
};

/**
 *  * HypervisorInterfaceCollection class to handle the GET and PATCH on VMI
 * Interface
 *   */
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
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/hypervisor/EthernetInterfaces";
        res.jsonValue["Name"] =
            "Virtual Management Ethernet Network Interface Collection";
        res.jsonValue["Description"] = "Collection of Virtual Management "
                                       "Interfaces for the power hypervisor";
        nlohmann::json &iface_array = res.jsonValue["Members"];
        iface_array.push_back(
            {{"@odata.id",
              "/redfish/v1/Systems/hypervisor/EthernetInterfaces/intf0"}});
        iface_array.push_back(
            {{"@odata.id",
              "/redfish/v1/Systems/hypervisor/EthernetInterfaces/intf1"}});
        res.jsonValue["Members@odata.count"] = 2;
        res.end();
    }
};

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

inline bool extractVmiInterfaceData(const std::string &ethiface_id,
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
inline void
    extractVmiIPv4Data(const std::string &ethiface_id,
                       const GetManagedObjects &dbus_data,
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
                if (interface.first == "xyz.openbmc_project.Network.IP")
                {
                    // Instance IPv4AddressData structure, and set as
                    // appropriate
                    std::pair<
                        boost::container::flat_set<IPv4AddressData>::iterator,
                        bool>
                        it = ipv4_config.insert(IPv4AddressData{});
                    IPv4AddressData &ipv4_address = *it.first;
                    ipv4_address.id =
                        objpath.first.str.substr(ipv4PathStart.size());
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
 * Function that retrieves all properties for given VMI Ethernet Interface
 * Object from Settings Manager
 * @param ethiface_id a eth interface id to query on DBus
 * @param callback a function that shall be called to convert Dbus output
 * into JSON
 */
template <typename CallbackFunc>
void getVmiIfaceData(const std::string &ethiface_id, CallbackFunc &&callback)
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

            bool found = extractVmiInterfaceData(ethiface_id, resp, ethData);
            if (!found)
            {
                BMCWEB_LOG_DEBUG << "Not found";
                callback(false, ethData, ipv4Data);
                return;
            }
            extractVmiIPv4Data(ethiface_id, resp, ipv4Data);
            // Finally make a callback with usefull data
            callback(true, ethData, ipv4Data);
        },
        "xyz.openbmc_project.Settings", "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
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

        if (!ethData.hostname.empty())
        {
            json_response["HostName"] = ethData.hostname;
        }

        nlohmann::json &ipv4_array = json_response["IPv4Addresses"];
        nlohmann::json &ipv4_static_array =
            json_response["IPv4StaticAddresses"];
        ipv4_array = nlohmann::json::array();
        ipv4_static_array = nlohmann::json::array();
        for (auto &ipv4_config : ipv4Data)
        {
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

        getVmiIfaceData(
            params[0],
            [this, asyncResp, iface_id{std::string(params[0])}](
                const bool &success, const EthernetInterfaceData &ethData,
                const boost::container::flat_set<IPv4AddressData> &ipv4Data) {
                if (!success)
                {
                    messages::resourceNotFound(
                        asyncResp->res, "HostEthernetInterface", iface_id);
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
};
} // namespace redfish
