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

} // namespace redfish
