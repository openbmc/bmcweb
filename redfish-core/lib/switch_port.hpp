// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "fabric.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/metrics_util.hpp"
#include "utils/pcie_util.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

inline void handleFabricSwitchPortPathPortMetricsGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId, const std::string& portPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#PortMetrics.v1_3_0.PortMetrics";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}/Metrics", fabricId,
        switchId, portId);
    asyncResp->res.jsonValue["Id"] = "Metrics";
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port Metrics", switchId, portId);

    metrics_util::getPortPCIeMetrics(asyncResp, portPath);
}

inline void handleFabricSwitchPortPathPortGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId, const std::string& portPath,
    const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Port.v1_4_0.Port";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}",
                            fabricId, switchId, portId);
    asyncResp->res.jsonValue["Id"] = portId;
    asyncResp->res.jsonValue["Name"] =
        std::format("{} {} Port", switchId, portId);

    nlohmann::json& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Metrics"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}/Metrics", fabricId,
        switchId, portId);

    dbus::utility::getAllProperties(
        serviceName, portPath, "xyz.openbmc_project.Inventory.Connector.Port",
        std::bind_front(pcie_util::afterGetPortProperties, asyncResp));
}

inline void afterHandleFabricSwitchPortPaths(
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

inline void getAssociatedPortPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& portId,
    std::function<void(const std::string& portPath,
                       const std::string& serviceName)>&& callback,
    const std::string& switchPath)
{
    std::string associationPath = switchPath + "/connecting";
    dbus::utility::getAssociatedSubTree(
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Connector.Port"},
        std::bind_front(afterHandleFabricSwitchPortPaths, asyncResp, portId,
                        std::move(callback)));
}

inline void handleFabricSwitchPathPortCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
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
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports", fabricId, switchId);
    asyncResp->res.jsonValue["Name"] = switchId + " Port Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json::array_t members;
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::object_path(path).filename();
        nlohmann::json::object_t member;
        member["@odata.id"] =
            boost::urls::format("/redfish/v1/Fabrics/{}/Switches/{}/Ports/{}",
                                fabricId, switchId, name);
        members.emplace_back(std::move(member));
    }

    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void getFabricSwitchPortPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& switchPath)
{
    std::string associationPath = switchPath + "/connecting";
    dbus::utility::getAssociatedSubTreePaths(
        associationPath,
        sdbusplus::object_path{"/xyz/openbmc_project/inventory"}, 0,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Connector.Port"},
        std::bind_front(handleFabricSwitchPathPortCollection, asyncResp,
                        fabricId, switchId));
}

inline void handleFabricSwitchPortPathsSwitchCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Fabrics/{}/Switches", fabricId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#SwitchCollection.SwitchCollection";
    asyncResp->res.jsonValue["Name"] = fabricId + " Switch Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json::array_t members;
    for (const std::string& path : object)
    {
        nlohmann::json::object_t member;
        std::string name = sdbusplus::object_path(path).filename();
        member["@odata.id"] = boost::urls::format(
            "/redfish/v1/Fabrics/{}/Switches/{}", fabricId, name);
        members.emplace_back(std::move(member));
    }

    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void handleFabricSwitchPortMetricsGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getFabricSwitchPath(
        asyncResp, fabricId, switchId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleFabricSwitchPortPathPortMetricsGet, asyncResp,
                            fabricId, switchId, portId)));
}

inline void handleFabricSwitchPortGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& portId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getFabricSwitchPath(
        asyncResp, fabricId, switchId,
        std::bind_front(
            getAssociatedPortPath, asyncResp, portId,
            std::bind_front(handleFabricSwitchPortPathPortGet, asyncResp,
                            fabricId, switchId, portId)));
}

inline void handleFabricSwitchPortsCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getFabricSwitchPath(asyncResp, fabricId, switchId,
                        std::bind_front(getFabricSwitchPortPaths, asyncResp,
                                        fabricId, switchId));
}

inline void requestRoutesFabricSwitchPort(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/")
        .privileges(redfish::privileges::getPortCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleFabricSwitchPortsCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/<str>/")
        .privileges(redfish::privileges::getPort)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchPortGet, std::ref(app)));

    BMCWEB_ROUTE(
        app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Ports/<str>/Metrics/")
        .privileges(redfish::privileges::getPortMetrics)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchPortMetricsGet, std::ref(app)));
}
} // namespace redfish
