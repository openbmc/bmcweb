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
     *      * Default Constructor
     *           */
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
                        const GetManagedObjects &dbus_data) {
                bool isFound = false;
                if (ec)
                {
                    isFound = false;
                }
                for (const auto &objpath : dbus_data)
                {
                    if (objpath.first == "/xyz/openbmc_project/network/vmi")
                    {
                        isFound = true;
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
                    }
                }
                if (isFound == false)
                {
                    messages::resourceNotFound(asyncResp->res, "System",
                                               "hypervisor");
                    return;
                }
            },
            "xyz.openbmc_project.Settings", "/",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const GetManagedObjects &dbus_data) {
                bool isFound = false;

                nlohmann::json &iface_array =
                    asyncResp->res.jsonValue["Members"];
                auto &count = asyncResp->res.jsonValue["Members@odata.count"];
                count = 0;

                if (ec)
                {
                    isFound = false;
                }
                for (const auto &objpath : dbus_data)
                {
                    if (objpath.first ==
                        "/xyz/openbmc_project/network/vmi/intf0")
                    {
                        isFound = true;
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
                        iface_array.push_back(
                            {{"@odata.id", "/redfish/v1/Systems/hypervisor/"
                                           "EthernetInterfaces/intf0"}});
                    }
                    if (objpath.first ==
                        "/xyz/openbmc_project/network/vmi/intf1")
                    {
                        isFound = true;
                        iface_array.push_back(
                            {{"@odata.id", "/redfish/v1/Systems/hypervisor/"
                                           "EthernetInterfaces/intf1"}});
                    }
                }
                if (isFound == false)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                count = iface_array.size();
            },
            "xyz.openbmc_project.Settings", "/",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
};

} // namespace redfish
