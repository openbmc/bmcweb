// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "generated/enums/resource.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"

static constexpr std::array<std::string_view, 1> fabricInterface = {
    "xyz.openbmc_project.Inventory.Item.Fabric"};
static constexpr std::array<std::string_view, 1> switchInterface = {
    "xyz.openbmc_project.Inventory.Item.Switch"};

namespace redfish
{

inline void handleFabricSwitchPathSwitchGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    [[maybe_unused]] const std::string& switchPath,
    [[maybe_unused]] const std::string& serviceName)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Switch.v1_7_0.Switch";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Fabrics/{}/Switches/{}", fabricId, switchId);
    asyncResp->res.jsonValue["Id"] = switchId;
    asyncResp->res.jsonValue["Name"] = switchId;

    auto& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["HealthRollup"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Ports"]["@odata.id"] = std::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports", fabricId, switchId);
}

inline void handleFabricSwitchPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchId,
    const std::function<void(const std::string& switchPath,
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

    std::string switchPath;
    std::string serviceName;
    for (const auto& [path, service] : object)
    {
        std::string switchName =
            sdbusplus::message::object_path(path).filename();
        if (switchName == switchId)
        {
            switchPath = path;
            if (service.size() != 1)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            serviceName = service.begin()->first;
            break;
        }
    }

    if (switchPath.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Switch", switchId);
        return;
    }

    callback(switchPath, serviceName);
}

inline void getFabricSwitchPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    std::function<void(const std::string& switchPath,
                       const std::string& serviceName)>&& callback)
{
    dbus::utility::getAssociatedSubTreeById(
        fabricId, "/xyz/openbmc_project/inventory", fabricInterface,
        "all_switches", switchInterface,
        std::bind_front(handleFabricSwitchPaths, asyncResp, switchId,
                        std::move(callback)));
}

inline void handleFabricSwitchPathsSwitchCollection(
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
        std::format("/redfish/v1/Fabrics/{}/Switches", fabricId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#SwitchCollection.SwitchCollection";
    asyncResp->res.jsonValue["Name"] = fabricId + " Switch Collection";

    asyncResp->res.jsonValue["Members@odata.count"] = object.size();

    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    members = nlohmann::json::array();
    for (const std::string& path : object)
    {
        std::string name = sdbusplus::message::object_path(path).filename();
        nlohmann::json member;
        member["@odata.id"] =
            std::format("/redfish/v1/Fabrics/{}/Switches/{}", fabricId, name);
        members.push_back(std::move(member));
    }
}

inline void handleFabricSwitchGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    getFabricSwitchPath(asyncResp, fabricId, switchId,
                        std::bind_front(handleFabricSwitchPathSwitchGet,
                                        asyncResp, fabricId, switchId));
}

inline void handleFabricSwitchCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    dbus::utility::getAssociatedSubTreePathsById(
        fabricId, "/xyz/openbmc_project/inventory", fabricInterface,
        "all_switches", switchInterface,
        std::bind_front(handleFabricSwitchPathsSwitchCollection, asyncResp,
                        fabricId));
}

inline void afterHandleFabricGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus response error on GetSubTree {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::string fabricPath;
    for (const auto& [path, service] : object)
    {
        std::string fabricName =
            sdbusplus::message::object_path(path).filename();
        if (fabricName == fabricId)
        {
            fabricPath = path;
            break;
        }
    }

    if (fabricPath.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Fabric", fabricId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#Fabric.v1_2_0.Fabric";
    asyncResp->res.jsonValue["@odata.id"] =
        std::format("/redfish/v1/Fabrics/{}", fabricId);
    asyncResp->res.jsonValue["Id"] = fabricId;
    asyncResp->res.jsonValue["Name"] = fabricId + " Fabric";

    asyncResp->res.jsonValue["Switches"]["@odata.id"] =
        std::format("/redfish/v1/Fabrics/{}/Switches", fabricId);
}

inline void handleFabricGet(crow::App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& fabricId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, fabricInterface,
        std::bind_front(afterHandleFabricGet, asyncResp, fabricId));
}

inline void handleFabricCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Fabrics";
    asyncResp->res.jsonValue["@odata.type"] =
        "#FabricCollection.FabricCollection";

    asyncResp->res.jsonValue["Name"] = "Fabric Collection";

    collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/redfish/v1/Fabrics"), fabricInterface,
        "/xyz/openbmc_project/inventory");
}

inline void requestRoutesFabrics(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/")
        .privileges(redfish::privileges::getComputerSystemCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/")
        .privileges(redfish::privileges::getComputerSystem)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/")
        .privileges(redfish::privileges::getComputerSystem)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/")
        .privileges(redfish::privileges::getComputerSystem)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchGet, std::ref(app)));
}
} // namespace redfish
