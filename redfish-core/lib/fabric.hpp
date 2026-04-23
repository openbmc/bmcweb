// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/collection.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <cerrno>
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

inline std::optional<resource::ResetType> dbusControlResetTypeToRf(
    const std::string& dbusValue)
{
    if (dbusValue ==
        "xyz.openbmc_project.Control.Reset.ResetTypes.ForceRestart")
    {
        return resource::ResetType::ForceRestart;
    }
    return std::nullopt;
}

inline void afterGetSwitchResetTypeForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchId, const boost::system::error_code& ec,
    const std::string& dbusResetType)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG(
            "Unable to read Control.Reset.ResetType for Switch {}: {}",
            switchId, ec.value());
        return;
    }

    std::optional<resource::ResetType> rfType =
        dbusControlResetTypeToRf(dbusResetType);
    if (!rfType)
    {
        BMCWEB_LOG_WARNING(
            "Unknown Control.Reset.ResetType value '{}' for Switch {}",
            dbusResetType, switchId);
        return;
    }

    nlohmann::json supportedJson = *rfType;
    nlohmann::json::array_t allowableValues;
    allowableValues.emplace_back(supportedJson.get<std::string>());
    asyncResp->res.jsonValue["Actions"]["#Switch.Reset"]
                            ["ResetType@Redfish.AllowableValues"] =
        std::move(allowableValues);
}

inline void afterGetSwitchResetControlForGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec || subtree.empty())
    {
        return;
    }
    if (subtree.size() > 1)
    {
        BMCWEB_LOG_ERROR(
            "Multiple Control.Reset objects in controlled_by association for Switch {}",
            switchId);
        return;
    }

    const auto& [resetPath, serviceMap] = subtree[0];
    if (serviceMap.size() != 1)
    {
        BMCWEB_LOG_ERROR("Unexpected service count {} for Control.Reset at {}",
                         serviceMap.size(), resetPath);
        return;
    }
    const std::string& service = serviceMap[0].first;

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, resetPath,
        "xyz.openbmc_project.Control.Reset", "ResetType",
        std::bind_front(afterGetSwitchResetTypeForGet, asyncResp, switchId));
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

    asyncResp->res.jsonValue["Actions"]["#Switch.Reset"]["target"] =
        boost::urls::format(
            "/redfish/v1/Fabrics/{}/Switches/{}/Actions/Switch.Reset", fabricId,
            switchId);

    constexpr std::array<std::string_view, 1> resetInterfaces = {
        "xyz.openbmc_project.Control.Reset"};
    dbus::utility::getAssociatedSubTree(
        sdbusplus::object_path(switchPath) / "controlled_by",
        sdbusplus::object_path("/xyz/openbmc_project/control/reset"), 0,
        resetInterfaces,
        std::bind_front(afterGetSwitchResetControlForGet, asyncResp, switchId));
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

inline void afterResetMethodCall(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchId, const boost::system::error_code& ec)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("Switch {} Reset failed: {}", switchId, ec.message());
        messages::internalError(asyncResp->res);
        return;
    }
    messages::success(asyncResp->res);
}

inline void afterGetSwitchResetTypeForPost(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchId, const std::string& requestedResetType,
    const std::string& service, const std::string& resetPath,
    const boost::system::error_code& ec, const std::string& dbusResetType)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR(
            "Unable to read Control.Reset.ResetType for Switch {}: {}",
            switchId, ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<resource::ResetType> rfType =
        dbusControlResetTypeToRf(dbusResetType);
    if (!rfType)
    {
        BMCWEB_LOG_ERROR(
            "Unknown Control.Reset.ResetType value '{}' for Switch {}",
            dbusResetType, switchId);
        messages::internalError(asyncResp->res);
        return;
    }

    nlohmann::json supportedJson = *rfType;
    const std::string supportedStr = supportedJson.get<std::string>();

    if (requestedResetType != supportedStr)
    {
        BMCWEB_LOG_DEBUG(
            "ResetType '{}' not supported for Switch {} (service reports '{}')",
            requestedResetType, switchId, supportedStr);
        messages::actionParameterNotSupported(asyncResp->res,
                                              requestedResetType, "ResetType");
        return;
    }

    crow::connections::systemBus->async_method_call(
        std::bind_front(afterResetMethodCall, asyncResp, switchId), service,
        resetPath, "xyz.openbmc_project.Control.Reset", "Reset");
}

inline void afterGetSwitchResetControlForPost(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& switchId, const std::string& requestedResetType,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() == EBADR || ec == boost::system::errc::io_error)
        {
            messages::resourceNotFound(asyncResp->res, "Switch", switchId);
            return;
        }
        BMCWEB_LOG_ERROR("DBus error looking up Control.Reset for {}: {}",
                         switchId, ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    if (subtree.empty())
    {
        messages::resourceNotFound(asyncResp->res, "Switch", switchId);
        return;
    }

    if (subtree.size() > 1)
    {
        BMCWEB_LOG_ERROR(
            "Multiple Control.Reset objects in controlled_by association for Switch {}",
            switchId);
        messages::internalError(asyncResp->res);
        return;
    }

    const auto& [resetPath, serviceMap] = subtree[0];
    if (serviceMap.size() != 1)
    {
        BMCWEB_LOG_ERROR("Unexpected service count {} for Control.Reset at {}",
                         serviceMap.size(), resetPath);
        messages::internalError(asyncResp->res);
        return;
    }
    const std::string& service = serviceMap[0].first;

    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service, resetPath,
        "xyz.openbmc_project.Control.Reset", "ResetType",
        std::bind_front(afterGetSwitchResetTypeForPost, asyncResp, switchId,
                        requestedResetType, service, resetPath));
}

inline void doSwitchReset(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& switchId,
                          const std::string& requestedResetType,
                          const std::string& switchPath)
{
    constexpr std::array<std::string_view, 1> resetInterfaces = {
        "xyz.openbmc_project.Control.Reset"};

    dbus::utility::getAssociatedSubTree(
        sdbusplus::object_path(switchPath) / "controlled_by",
        sdbusplus::object_path("/xyz/openbmc_project/control/reset"), 0,
        resetInterfaces,
        std::bind_front(afterGetSwitchResetControlForPost, asyncResp, switchId,
                        requestedResetType));
}

inline void handleFabricSwitchResetPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fabricId, const std::string& switchId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<std::string> resetType;
    if (!json_util::readJsonAction(req, asyncResp->res, "ResetType", resetType))
    {
        return;
    }

    // ResetType is optional per the Redfish schema. When omitted, default to
    // ForceRestart. The actual supported value is reported by the downstream
    // Control.Reset service via its ResetType property and is validated
    // against the client request.
    const std::string requestedResetType = resetType.value_or("ForceRestart");

    getFabricSwitchPath(asyncResp, fabricId, switchId,
                        std::bind_front(doSwitchReset, asyncResp, switchId,
                                        requestedResetType));
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

    BMCWEB_ROUTE(
        app, "/redfish/v1/Fabrics/<str>/Switches/<str>/Actions/Switch.Reset/")
        .privileges(redfish::privileges::postSwitch)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleFabricSwitchResetPost, std::ref(app)));
}
} // namespace redfish
