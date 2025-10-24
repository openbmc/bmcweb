// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "switch_port.hpp"
#include "utils/chassis_utils.hpp"

#include <boost/beast/http/verb.hpp>

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

static constexpr std::array<std::string_view, 1> networkAdapterInterface = {
    "xyz.openbmc_project.Inventory.Item.NetworkAdapter"};

namespace redfish
{

inline void handleNetworkAdapterPortPathPortMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId, const std::string& portPath,
    const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#PortMetrics.v1_7_0.PortMetrics";
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}/Metrics", chassisId,
        networkAdapterId, portId);
    asyncResp->res.jsonValue["Id"] = "Metrics";
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port Metrics", networkAdapterId, portId);

    getMetricProperty(asyncResp, serviceName, portPath, "/network/rx_bytes",
                      "/RXBytes");
    getMetricProperty(asyncResp, serviceName, portPath, "/network/tx_bytes",
                      "/TXBytes");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_multicast_frames",
                      "/Networking/RXUnicastFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_multicast_frames",
                      "/Networking/RXMulticastFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_broadcast_frames",
                      "/Networking/RXBroadcastFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_unicast_frames",
                      "/Networking/TXUnicastFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_multicast_frames",
                      "/Networking/TXMulticastFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_broadcast_frames",
                      "/Networking/TXBroadcastFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_fcs_errors", "/Networking/RXFCSErrors");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_frame_alignment_errors",
                      "/Networking/RXFrameAlignmentErrors");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_false_carrier_errors",
                      "/Networking/RXFalseCarrierErrors");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_undersize_frames",
                      "/Networking/RXUndersizeFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_oversize_frames",
                      "/Networking/RXOversizeFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_pause_xon_frames",
                      "/Networking/RXPauseXONFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/rx_pause_xoff_frames",
                      "/Networking/RXPauseXOFFFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_pause_xon_frames",
                      "/Networking/TXPauseXONFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_pause_xoff_frames",
                      "/Networking/TXPauseXOFFFrames");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_single_collisions",
                      "/Networking/TXSingleCollisions");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_multiple_collisions",
                      "/Networking/TXMultipleCollisions");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_late_collisions",
                      "/Networking/TXLateCollisions");
    getMetricProperty(asyncResp, serviceName, portPath,
                      "/network/tx_excessive_collisions",
                      "/Networking/TXExcessiveCollisions");
}

inline void handleNetworkAdapterPathPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassiId, const std::string& networkAdapterId,
    const std::string& portId, [[maybe_unused]] const std::string& portPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_9_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}",
                    chassiId, networkAdapterId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port", networkAdapterId, portId);

    auto& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["HealthRollup"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Metrics"]["@odata.id"] = std::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}/Metrics", chassiId,
        networkAdapterId, portId);
}

inline void afterNetworkAdapterPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    const std::function<void(const std::string& portPath,
                             const std::string& serviceName)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTreeById {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::string portPath;
    std::string serviceName;
    for (const auto& [path, service] : object)
    {
        std::string portName = sdbusplus::message::object_path(path).filename();
        if (portName == portId)
        {
            portPath = path;
            if (service.size() != 1)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            serviceName = service.begin()->first;
            break;
        }
    }

    if (portPath.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Port", portId);
        return;
    }

    callback(portPath, serviceName);
}

inline void getNetworkAdapterPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& serviceName)>&& callback,
    const std::string& path, [[maybe_unused]] const std::string& serviceName)
{
    std::string associationPath = path + "/connecting";
    dbus::utility::getAssociatedSubTree(
        associationPath, sdbusplus::message::object_path{inventoryPath}, 0,
        portInterface,
        std::bind_front(afterNetworkAdapterPortPaths, asyncResp, portId,
                        std::move(callback)));
}

inline void handleNetworkAdapterPortPathPortCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#PortCollection.PortCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports",
                    chassisId, networkAdapterId);
    asyncResp->res.jsonValue["Name"] = networkAdapterId + " Port Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::message::object_path(path).filename();
        nlohmann::json member;
        member["@odata.id"] =
            std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}",
                        chassisId, networkAdapterId, name);
        members.push_back(std::move(member));
    }
}

inline void getNetworkAdapterPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& networkAdapterPath)
{
    std::string associationPath = networkAdapterPath + "/connecting";
    dbus::utility::getAssociatedSubTreePaths(
        associationPath, sdbusplus::message::object_path{inventoryPath}, 0,
        portInterface,
        std::bind_front(handleNetworkAdapterPortPathPortCollection, asyncResp,
                        chassisId, networkAdapterId));
}

inline void handleNetworkAdapterPathNetworkAdapterGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    [[maybe_unused]] const std::string& path)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#NetworkAdapter.v1_11_0.NetworkAdapter";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}", chassisId,
                    networkAdapterId);
    asyncResp->res.jsonValue["Id"] = networkAdapterId;
    asyncResp->res.jsonValue["Name"] = networkAdapterId + " Network Adapter";

    auto& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["HealthRollup"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Ports"]["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports",
                    chassisId, networkAdapterId);
}

inline void handleNetworkAdapterPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& networkAdapterId,
    const std::function<void(const std::string& path)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTreeById {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& path : object)
    {
        std::string name = sdbusplus::message::object_path(path).filename();
        if (name == networkAdapterId)
        {
            callback(path);
            return;
        }
    }

    messages::resourceNotFound(asyncResp->res, "NetworkAdapter",
                               networkAdapterId);
}

inline void getNetworkAdapterPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    std::function<void(const std::string& path)>&& callback)
{
    dbus::utility::getAssociatedSubTreePathsById(
        chassisId, "/xyz/openbmc_project/inventory", chassisInterfaces,
        "containing", networkAdapterInterface,
        std::bind_front(handleNetworkAdapterPaths, asyncResp, networkAdapterId,
                        std::move(callback)));
}

inline void handleNetworkAdapterPathsNetworkAdapterCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters", chassisId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#NetworkAdapterCollection.NetworkAdapterCollection";
    asyncResp->res.jsonValue["Name"] =
        chassisId + " Network Adapter Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::message::object_path(path).filename();
        nlohmann::json member;
        member["@odata.id"] = std::format(
            "/redfish/v1/Chassis/{}/NetworkAdapters/{}", chassisId, name);
        members.push_back(std::move(member));
    }
}

inline void handleNetworkAdapterGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        std::bind_front(handleNetworkAdapterPathNetworkAdapterGet, asyncResp,
                        chassisId, networkAdapterId));
}

inline void handleNetworkAdapterCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    dbus::utility::getAssociatedSubTreePathsById(
        chassisId, "/xyz/openbmc_project/inventory", chassisInterfaces,
        "containing", networkAdapterInterface,
        std::bind_front(handleNetworkAdapterPathsNetworkAdapterCollection,
                        asyncResp, chassisId));
}

inline void handleNetworkAdapterPortMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleNetworkAdapterPortPathPortMetricsGet,
                            asyncResp, chassisId, networkAdapterId, portId)));
}

inline void handleNetworkAdapterPortGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(
        asyncResp, chassisId, networkAdapterId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleNetworkAdapterPathPortGet, asyncResp,
                            chassisId, networkAdapterId, portId)));
}

inline void handleNetworkAdapterPortCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getNetworkAdapterPath(asyncResp, chassisId, networkAdapterId,
                          std::bind_front(getNetworkAdapterPortPaths, asyncResp,
                                          chassisId, networkAdapterId));
}

inline void requestRoutesChassisNetworkAdapter(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/NetworkAdapters/")
        .privileges(redfish::privileges::getNetworkAdapterCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/")
        .privileges(redfish::privileges::getNetworkAdapter)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleNetworkAdapterPortCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterPortGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/NetworkAdapters/<str>/Ports/<str>/Metrics/")
        .privileges(redfish::privileges::getPortMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleNetworkAdapterPortMetricsGet, std::ref(app)));
}
} // namespace redfish
