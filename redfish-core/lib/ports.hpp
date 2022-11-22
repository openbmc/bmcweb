#pragma once

#include "ethernet.hpp"
#include "manager_diagnostic_data.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <app.hpp>
#include <boost/container/flat_set.hpp>

#include <format>

namespace redfish
{

struct PortInfo
{
    int portId;
    std::string interfaceName;
    int speed;
    bool enabled;
    bool linkUp;
    uint TXDropped;
    uint RXDropped;
};

int getSpeed(std::string interfaceName)
{
    const std::string& speedPath = "/sys/class/net/" + interfaceName + "/speed";
    const std::string speedFile = readFileIntoString(speedPath);
    return std::atoi(speedFile.c_str());
}

bool getEnabled(std::string interfaceName)
{
    const std::string& dormantPath =
        "/sys/class/net/" + interfaceName + "/dormant";
    const std::string dormant = readFileIntoString(dormantPath);
    // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net
    return std::atoi(dormant.c_str()) == 0 ? true : false;
}

bool getLinkUp(std::string interfaceName)
{
    const std::string& linkUpPath =
        "/sys/class/net/" + interfaceName + "/carrier";
    const std::string linkUp = readFileIntoString(linkUpPath);
    // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net
    return std::atoi(linkUp.c_str()) == 0 ? false : true;
}

PortInfo getPortInfo(const int portId)
{
    struct ifaddrs* ifaddr;
    int family;

    if (getifaddrs(&ifaddr) == -1)
    {
        BMCWEB_LOG_ERROR << "getifaddr error";
        exit(EXIT_FAILURE);
    }

    PortInfo portInfo;
    int portNum = 0;

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        family = ifa->ifa_addr->sa_family;

        if (family != AF_PACKET || ifa->ifa_data == NULL)
        {
            continue;
        }

        if (portNum == portId)
        {
            portInfo.portId = portNum;
            portInfo.interfaceName = ifa->ifa_name;
            portInfo.speed = getSpeed(ifa->ifa_name);
            portInfo.enabled = getEnabled(ifa->ifa_name);
            portInfo.linkUp = getLinkUp(ifa->ifa_name);

            struct rtnl_link_stats* stats =
                static_cast<rtnl_link_stats*>(ifa->ifa_data);
            portInfo.TXDropped = stats->tx_dropped;
            portInfo.RXDropped = stats->rx_dropped;

            freeifaddrs(ifaddr);
            return portInfo;
        }

        // Only track AF_PACKETS
        ++portNum;
    }
    freeifaddrs(ifaddr);

    portInfo.portId = -1;
    return portInfo;
}

int getNumPorts()
{
    struct ifaddrs* ifaddr;
    int family;

    if (getifaddrs(&ifaddr) == -1)
    {
        BMCWEB_LOG_ERROR << "getifaddr error";
        exit(EXIT_FAILURE);
    }

    int numPorts = 0;

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        family = ifa->ifa_addr->sa_family;

        if (family != AF_PACKET || ifa->ifa_data == NULL)
        {
            continue;
        }

        // Only track AF_PACKETS
        ++numPorts;
    }
    freeifaddrs(ifaddr);
    return numPorts;
}

std::string getLinkState(bool linkUp, bool enabled)
{
    std::string linkUpState = linkUp ? "LinkUp" : "LinkDown";
    std::string enabledState = enabled ? "Enabled" : "Disabled";
    return linkUpState + " " + enabledState;
}

inline void handleDedicatedNetworkPortsGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const int& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_DEBUG << "Got port " << portId;

    PortInfo portInfo = getPortInfo(portId);

    // Check if this is a valid port
    if (portInfo.portId == -1)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_7_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/DedicatedNetworkPorts/" +
        std::to_string(portId);
    asyncResp->res.jsonValue["Name"] = portInfo.interfaceName;
    asyncResp->res.jsonValue["Id"] = portInfo.portId;
    asyncResp->res.jsonValue["PortId"] = portInfo.portId;
    asyncResp->res.jsonValue["CurrentSpeedGbps"] = portInfo.speed;
    asyncResp->res.jsonValue["LinkState"] =
        getLinkState(portInfo.linkUp, portInfo.enabled);
    asyncResp->res.jsonValue["Description"] = "Port information";
    asyncResp->res.jsonValue["Metrics"]["@odata.id"] =
        "/redfish/v1/Managers/bmc/DedicatedNetworkPorts/" +
        std::to_string(portId) + "/Metrics";
}

inline void handleDedicatedNetworkPortsCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/DedicatedNetworkPorts";
    asyncResp->res.jsonValue["Name"] = "Dedicated Network Ports Collection";
    asyncResp->res.jsonValue["Description"] =
        "Collection of Dedicated Network Ports for this Manager";

    int numPorts = getNumPorts();

    asyncResp->res.jsonValue["Members@odata.count"] = numPorts;
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/DedicatedNetworkPorts";

    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();

    for (int i = 0; i < numPorts; ++i)
    {
        nlohmann::json portInterface;
        portInterface["@odata.id"] =
            "/redfish/v1/Managers/bmc/DedicatedNetworkPorts/" +
            std::to_string(i);
        asyncResp->res.jsonValue["Members"].push_back(portInterface);
    }
}

inline void requestPortsRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/DedicatedNetworkPorts")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleDedicatedNetworkPortsCollectionGet, std::ref(app)));
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/DedicatedNetworkPorts/<int>")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleDedicatedNetworkPortsGet, std::ref(app)));
}

} // namespace redfish