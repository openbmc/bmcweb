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
#include "utils/collection.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/url.hpp>

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

static constexpr std::array<std::string_view, 1> switchInterfaces = {
    "xyz.openbmc_project.Inventory.Item.PCIeSwitch"};

namespace redfish
{

inline std::optional<resource::PowerState> dbusToRfPowerState(
    std::string_view dbusPowerState)
{
    if (dbusPowerState ==
        "xyz.openbmc_project.State.Decorator.PowerState.State.On")
    {
        return resource::PowerState::On;
    }
    if (dbusPowerState ==
        "xyz.openbmc_project.State.Decorator.PowerState.State.Off")
    {
        return resource::PowerState::Off;
    }
    if (dbusPowerState ==
        "xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOn")
    {
        return resource::PowerState::PoweringOn;
    }
    if (dbusPowerState ==
        "xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOff")
    {
        return resource::PowerState::PoweringOff;
    }
    return std::nullopt;
}

inline void afterGetSwitchPowerState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchPath, const boost::system::error_code& ec,
    const std::string& powerState)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus error reading PowerState on {}: {}", switchPath,
                         ec.message());
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<resource::PowerState> rfState =
        dbusToRfPowerState(powerState);
    if (!rfState)
    {
        BMCWEB_LOG_WARNING("Unknown PowerState '{}' on {}", powerState,
                           switchPath);
        return;
    }
    asyncResp->res.jsonValue["PowerState"] = *rfState;
}

inline void afterGetSwitchPowerStateService(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& obj)
{
    if (ec.value() == EBADR || ec.value() == boost::system::errc::io_error)
    {
        // PowerState decorator is optional on a Switch; no service
        // implements the interface on this path, so leave PowerState
        // out of the response.
        BMCWEB_LOG_DEBUG("PowerState interface absent on {}", switchPath);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR("Mapper GetObject error for {}: {}", switchPath,
                         ec.message());
        messages::internalError(asyncResp->res);
        return;
    }
    if (obj.empty())
    {
        return;
    }

    const std::string& service = obj.begin()->first;
    dbus::utility::getProperty<std::string>(
        service, switchPath, "xyz.openbmc_project.State.Decorator.PowerState",
        "PowerState",
        std::bind_front(afterGetSwitchPowerState, asyncResp, switchPath));
}

inline void handleFabricSwitchPathSwitchGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    const std::string& switchPath)
{
    asyncResp->res.jsonValue["@odata.type"] = "#Switch.v1_7_0.Switch";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}", fabricId, switchId);
    asyncResp->res.jsonValue["Id"] = switchId;
    asyncResp->res.jsonValue["Name"] = switchId;

    nlohmann::json& status = asyncResp->res.jsonValue["Status"];
    status["Health"] = resource::Health::OK;
    status["State"] = resource::State::Enabled;

    asyncResp->res.jsonValue["Ports"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Fabrics/{}/Switches/{}/Ports", fabricId, switchId);

    constexpr std::array<std::string_view, 1> powerStateInterface = {
        "xyz.openbmc_project.State.Decorator.PowerState"};
    dbus::utility::getDbusObject(
        switchPath, powerStateInterface,
        std::bind_front(afterGetSwitchPowerStateService, asyncResp,
                        switchPath));
}

inline void handleFabricSwitchPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchId,
    const std::function<void(const std::string& switchPath)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& object)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error || ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("Switch resource {} not found", switchId);
            messages::resourceNotFound(asyncResp->res, "Switch", switchId);
            return;
        }

        BMCWEB_LOG_ERROR("DBus response error on GetSubTreePaths {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::string switchPath;

    for (const auto& path : object)
    {
        std::string switchName = sdbusplus::object_path(path).filename();
        if (switchName == switchId)
        {
            if (!switchPath.empty())
            {
                BMCWEB_LOG_ERROR("Multiple Switch resources found for {}",
                                 switchId);
                messages::internalError(asyncResp->res);
                return;
            }

            switchPath = path;
        }
    }

    if (!switchPath.empty())
    {
        callback(switchPath);
        return;
    }

    messages::resourceNotFound(asyncResp->res, "Switch", switchId);
}

inline void getFabricSwitchPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId,
    std::function<void(const std::string& switchPath)>&& callback)
{
    if (fabricId != BMCWEB_REDFISH_FABRIC_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Fabric", fabricId);
        return;
    }

    dbus::utility::getSubTreePaths(
        "/xyz/openbmc_project/inventory", 0, switchInterfaces,
        std::bind_front(handleFabricSwitchPaths, asyncResp, switchId,
                        std::move(callback)));
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

    if (fabricId != BMCWEB_REDFISH_FABRIC_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Fabric", fabricId);
        return;
    }

    const auto switchUrl =
        boost::urls::format("/redfish/v1/Fabrics/{}/Switches", fabricId);

    asyncResp->res.jsonValue["@odata.id"] = switchUrl;
    asyncResp->res.jsonValue["@odata.type"] =
        "#SwitchCollection.SwitchCollection";
    asyncResp->res.jsonValue["Name"] = fabricId + " Switch Collection";

    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Fabrics/{}/Switches", fabricId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#SwitchCollection.SwitchCollection";
    asyncResp->res.jsonValue["Name"] = fabricId + " Switch Collection";

    collection_util::getCollectionMembers(
        asyncResp, switchUrl, switchInterfaces,
        "/xyz/openbmc_project/inventory");
}

inline void handleFabricGet(crow::App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& fabricId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (fabricId != BMCWEB_REDFISH_FABRIC_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "Fabric", fabricId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#Fabric.v1_2_0.Fabric";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Fabrics/{}", fabricId);
    asyncResp->res.jsonValue["Id"] = fabricId;
    asyncResp->res.jsonValue["Name"] = fabricId + " Fabric";

    asyncResp->res.jsonValue["Switches"]["@odata.id"] =
        boost::urls::format("/redfish/v1/Fabrics/{}/Switches", fabricId);
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

    asyncResp->res.jsonValue["Members@odata.count"] = 1;

    nlohmann::json::array_t members;
    nlohmann::json::object_t member;
    member["@odata.id"] = boost::urls::format("/redfish/v1/Fabrics/{}",
                                              BMCWEB_REDFISH_FABRIC_URI_NAME);
    members.emplace_back(std::move(member));

    asyncResp->res.jsonValue["Members"] = std::move(members);
}

inline void requestRoutesFabrics(App& app)
{
    // The Fabric resource is designed so that each BMC will have only one
    // Fabric resource.
    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/")
        .privileges(redfish::privileges::getFabricCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/")
        .privileges(redfish::privileges::getFabric)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/")
        .privileges(redfish::privileges::getSwitchCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Fabrics/<str>/Switches/<str>/")
        .privileges(redfish::privileges::getSwitch)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleFabricSwitchGet, std::ref(app)));
}
} // namespace redfish
