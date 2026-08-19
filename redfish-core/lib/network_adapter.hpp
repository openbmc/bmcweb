// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/port.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "switch_port.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/resource_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>

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

inline void handleNetworkAdapterPortMetricsPathsPortMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetAssociatedSubTree{}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [path, service] : object)
    {
        if (service.size() != 1)
        {
            continue;
        }

        sdbusplus::object_path objectPah(path);

        const std::string metricType = objectPah.parent_path().filename();
        const std::string metricName = objectPah.filename();

        if (metricType != "nic")
        {
            continue;
        }

        const auto& serviceName = service.begin()->first;

        if (metricName == "rx_bytes")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/RXBytes"_json_pointer);
        }
        else if (metricName == "tx_bytes")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/TXBytes"_json_pointer);
        }
        else if (metricName == "rx_multicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXMulticastFrames"_json_pointer);
        }
        else if (metricName == "tx_multicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXMulticastFrames"_json_pointer);
        }
        else if (metricName == "rx_broadcast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXBroadcastFrames"_json_pointer);
        }
        else if (metricName == "tx_broadcast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXBroadcastFrames"_json_pointer);
        }
        else if (metricName == "rx_unicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXUnicastFrames"_json_pointer);
        }
        else if (metricName == "tx_unicast_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXUnicastFrames"_json_pointer);
        }
        else if (metricName == "rx_fcs_errors")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXFCSErrors"_json_pointer);
        }
        else if (metricName == "rx_frame_alignment_errors")
        {
            getMetricProperty(
                asyncResp, serviceName, path,
                "/Networking/RXFrameAlignmentErrors"_json_pointer);
        }
        else if (metricName == "rx_false_carrier_errors")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXFalseCarrierErrors"_json_pointer);
        }
        else if (metricName == "rx_undersize_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXUndersizeFrames"_json_pointer);
        }
        else if (metricName == "rx_oversize_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXOversizeFrames"_json_pointer);
        }
        else if (metricName == "rx_pause_xon_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXPauseXONFrames"_json_pointer);
        }
        else if (metricName == "rx_pause_xoff_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/RXPauseXOFFFrames"_json_pointer);
        }
        else if (metricName == "tx_pause_xon_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXPauseXONFrames"_json_pointer);
        }
        else if (metricName == "tx_pause_xoff_frames")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXPauseXOFFFrames"_json_pointer);
        }
        else if (metricName == "tx_single_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXSingleCollisions"_json_pointer);
        }
        else if (metricName == "tx_multiple_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXMultipleCollisions"_json_pointer);
        }
        else if (metricName == "tx_late_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXLateCollisions"_json_pointer);
        }
        else if (metricName == "tx_excessive_collisions")
        {
            getMetricProperty(asyncResp, serviceName, path,
                              "/Networking/TXExcessiveCollisions"_json_pointer);
        }
    }
}

inline void handleNetworkAdapterPortPathPortMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& portId, const sdbusplus::object_path& portPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#PortMetrics.v1_7_0.PortMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}/Metrics", chassisId,
        networkAdapterId, portId);
    asyncResp->res.jsonValue["Id"] = "Metrics";
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port Metrics", networkAdapterId, portId);

    const std::string associationPath = portPath / "measured_by";
    dbus::utility::getAssociatedSubTree(
        associationPath, sdbusplus::object_path("/xyz/openbmc_project/metric"),
        0, std::array<std::string_view, 1>{"xyz.openbmc_project.Metric.Value"},
        std::bind_front(handleNetworkAdapterPortMetricsPathsPortMetricsGet,
                        asyncResp));
}

inline void afterGetNetworkAdapterPortLinkStatus(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec, const std::string& linkStatus)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBus response error for LinkStatus {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    if (linkStatus ==
        "xyz.openbmc_project.State.Decorator.LinkStatus.Status.Connected")
    {
        asyncResp->res.jsonValue["LinkStatus"] = port::LinkStatus::LinkUp;
    }
    else if (
        linkStatus ==
        "xyz.openbmc_project.State.Decorator.LinkStatus.Status.Disconnected")
    {
        asyncResp->res.jsonValue["LinkStatus"] = port::LinkStatus::LinkDown;
    }
    else
    {
        BMCWEB_LOG_WARNING("Unknown LinkStatus {}", linkStatus);
    }
}

inline void getNetworkAdapterPortLinkStatus(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portPath, const std::string& serviceName)
{
    dbus::utility::getProperty<std::string>(
        serviceName, portPath, "xyz.openbmc_project.State.Decorator.LinkStatus",
        "LinkStatus",
        std::bind_front(afterGetNetworkAdapterPortLinkStatus, asyncResp));
}

inline void handleNetworkAdapterPathPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassiId, const std::string& networkAdapterId,
    const std::string& portId, const std::string& portPath,
    const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_9_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}",
                    chassiId, networkAdapterId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port", networkAdapterId, portId);

    asyncResp->res.jsonValue["Status"]["HealthRollup"] = resource::Health::OK;

    asyncResp->res.jsonValue["Metrics"]["@odata.id"] = std::format(
        "/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}/Metrics", chassiId,
        networkAdapterId, portId);

    resource_utils::getResourceState(asyncResp, serviceName, portPath,
                                     ""_json_pointer);
    resource_utils::getResourceHealth(asyncResp, serviceName, portPath,
                                      ""_json_pointer);
    getNetworkAdapterPortLinkStatus(asyncResp, portPath, serviceName);
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
        std::string portName = sdbusplus::object_path(path).filename();
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
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Connector.Port"},
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

    nlohmann::json::array_t members;
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::object_path(path).filename();
        nlohmann::json::object_t member;
        member["@odata.id"] =
            std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports/{}",
                        chassisId, networkAdapterId, name);
        members.emplace_back(std::move(member));
    }

    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void getNetworkAdapterPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& networkAdapterPath)
{
    std::string associationPath = networkAdapterPath + "/connecting";
    dbus::utility::getAssociatedSubTreePaths(
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Connector.Port"},
        std::bind_front(handleNetworkAdapterPortPathPortCollection, asyncResp,
                        chassisId, networkAdapterId));
}

inline void handleNetworkAdapterPathNetworkAdapterGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& path, const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#NetworkAdapter.v1_11_0.NetworkAdapter";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}", chassisId,
                    networkAdapterId);
    asyncResp->res.jsonValue["Id"] = networkAdapterId;
    asyncResp->res.jsonValue["Name"] = networkAdapterId + " Network Adapter";

    asyncResp->res.jsonValue["Status"]["HealthRollup"] = resource::Health::OK;

    asyncResp->res.jsonValue["Ports"]["@odata.id"] =
        std::format("/redfish/v1/Chassis/{}/NetworkAdapters/{}/Ports",
                    chassisId, networkAdapterId);

    resource_utils::getResourceState(asyncResp, serviceName, path,
                                     ""_json_pointer);
    resource_utils::getResourceHealth(asyncResp, serviceName, path,
                                      ""_json_pointer);
}

inline void afterGetNetworkAdapterService(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& path, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec || object.empty())
    {
        BMCWEB_LOG_ERROR("DBUS response error on getDbusObject {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    handleNetworkAdapterPathNetworkAdapterGet(
        asyncResp, chassisId, networkAdapterId, path, object.begin()->first);
}

inline void getNetworkAdapterService(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& networkAdapterId,
    const std::string& path)
{
    dbus::utility::getDbusObject(
        path, networkAdapterInterface,
        std::bind_front(afterGetNetworkAdapterService, asyncResp, chassisId,
                        networkAdapterId, path));
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
        std::string name = sdbusplus::object_path(path).filename();
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
        std::string name = sdbusplus::object_path(path).filename();
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

    getNetworkAdapterPath(asyncResp, chassisId, networkAdapterId,
                          std::bind_front(getNetworkAdapterService, asyncResp,
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
