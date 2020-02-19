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
        res.jsonValue["@odata.type"] = "#ComputerSystem.v1_6_0.ComputerSystem";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/hypervisor";
        res.jsonValue["Description"] = "Hypervisor";
        res.jsonValue["Name"] = "Hypervisor";
        res.jsonValue["Id"] = "hypervisor";
        res.jsonValue["Links"]["ManagedBy"] = {
            {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
        res.jsonValue["EthernetInterfaces"] = {
            {"@odata.id", "/redfish/v1/Systems/hypervisor/EthernetInterfaces"}};
        res.end();
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
        res.jsonValue["@odata.type"] =
            "#EthernetInterfaceCollection.EthernetInterfaceCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/hypervisor/EthernetInterfaces";
        res.jsonValue["Name"] =
            "Virtual Management Ethernet Network Interface Collection";
        res.jsonValue["Description"] = "Collection of Virtual Management "
                                       "Interfaces for the hypervisor";
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

} // namespace redfish
