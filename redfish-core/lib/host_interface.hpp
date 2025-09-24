// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "generated/enums/host_interface.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <string>
#include <unordered_map>

namespace redfish
{

using namespace host_interface;

constexpr std::string_view bmcRedfishHostIntfUriName = "host0";

inline std::string getCredentialRoleIdToUserRoleId(std::string_view role)
{
    if (role ==
        "xyz.openbmc_project.HostInterface.CredentialBootstrapping.Role.Administrator")
    {
        return "Administrator";
    }

    if (role ==
        "xyz.openbmc_project.HostInterface.CredentialBootstrapping.Role.Operator")
    {
        return "Operator";
    }

    if (role ==
        "xyz.openbmc_project.HostInterface.CredentialBootstrapping.Role.ReadOnly")
    {
        return "ReadOnly";
    }

    return "Administrator";
}

inline std::string getCredentialRoleIdFromUserRoleId(std::string_view role)
{
    if (role == "Administrator")
    {
        return "xyz.openbmc_project.HostInterface.CredentialBootstrapping.Role.Administrator";
    }

    if (role == "Operator")
    {
        return "xyz.openbmc_project.HostInterface.CredentialBootstrapping.Role.Operator";
    }

    if (role == "ReadOnly")
    {
        return "xyz.openbmc_project.HostInterface.CredentialBootstrapping.Role.ReadOnly";
    }

    return "xyz.openbmc_project.HostInterface.CredentialBootstrapping.Role.Administrator";
}

inline void handleHostInterfaceCollection(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (!BMCWEB_REDFISH_HOST_INTERFACE)
    {
        BMCWEB_LOG_WARNING("Redfish host interface is not supported.");
        messages::queryNotSupported(asyncResp->res);
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        BMCWEB_LOG_WARNING("Invalid manager Id {}.", managerId);
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/HostInterfaces", managerId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#HostInterfaceCollection.HostInterfaceCollection";
    asyncResp->res.jsonValue["Name"] = "HostInterface Collection";
    asyncResp->res.jsonValue["Description"] = "Collection of HostInterfaces";

    nlohmann::json::array_t memberArray;
    nlohmann::json::object_t member;
    member["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/HostInterfaces/{}",
                            managerId, bmcRedfishHostIntfUriName);
    memberArray.emplace_back(std::move(member));
    asyncResp->res.jsonValue["Members@odata.count"] = memberArray.size();
    asyncResp->res.jsonValue["Members"] = std::move(memberArray);
}

inline void onGetCredentialBootstrappingProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for Properties {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }
    const bool* enabledAfterReset = nullptr;
    const bool* enabled = nullptr;
    const std::string* roleId = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "EnableAfterReset",
        enabledAfterReset, "CredentialEnabled", enabled, "RoleId", roleId);
    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    nlohmann::json& creds = asyncResp->res.jsonValue["CredentialBootstrapping"];

    if (enabledAfterReset != nullptr)
    {
        creds["EnableAfterReset"] = *enabledAfterReset;
    }
    if (enabled != nullptr)
    {
        creds["Enabled"] = *enabled;
    }
    if (roleId != nullptr)
    {
        auto userRole = getCredentialRoleIdToUserRoleId(*roleId);
        creds["RoleId"] = userRole;
        asyncResp->res
            .jsonValue["Links"]["CredentialBootstrappingRole"]["@odata.id"] =
            boost::urls::format("/redfish/v1/AccountService/Roles/{}",
                                userRole);
    }
}

inline void handleHostInterfaceGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& hostInterfaceId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (!BMCWEB_REDFISH_HOST_INTERFACE)
    {
        BMCWEB_LOG_WARNING("Redfish host interface is not supported.");
        messages::queryNotSupported(asyncResp->res);
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        BMCWEB_LOG_WARNING("Invalid manager Id {}.", managerId);
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    if (hostInterfaceId != bmcRedfishHostIntfUriName)
    {
        BMCWEB_LOG_WARNING("Invalid Host Interface Id {}. Support {}.",
                           hostInterfaceId, bmcRedfishHostIntfUriName);
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/HostInterfaces", managerId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#HostInterface.v1_3_0.HostInterface";
    asyncResp->res.jsonValue["Description"] = "Host Interface";
    asyncResp->res.jsonValue["Name"] = "Host Interface";
    asyncResp->res.jsonValue["Id"] = hostInterfaceId;
    asyncResp->res.jsonValue["ExternallyAccessible"] = false;
    asyncResp->res.jsonValue["InterfaceEnabled"] = true;
    asyncResp->res.jsonValue["HostInterfaceType"] =
        HostInterfaceType::NetworkHostInterface;
    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
    asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;

    dbus::utility::getAllProperties(
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.HostInterface.CredentialBootstrapping",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
            onGetCredentialBootstrappingProperty(asyncResp, ec, properties);
        });
}

inline void handleHostInterfacePatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId, const std::string& hostInterfaceId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (!BMCWEB_REDFISH_HOST_INTERFACE)
    {
        BMCWEB_LOG_WARNING("Redfish host interface is not supported.");
        messages::queryNotSupported(asyncResp->res);
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        BMCWEB_LOG_WARNING("Invalid manager Id {}.", managerId);
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    if (hostInterfaceId != bmcRedfishHostIntfUriName)
    {
        BMCWEB_LOG_WARNING("Invalid Host Interface Id {}. Support {}.",
                           hostInterfaceId, bmcRedfishHostIntfUriName);
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    std::optional<bool> enabled;
    std::optional<bool> enableAfterReset;
    std::optional<std::string> roleId;

    if (!json_util::readJsonPatch(
            req, asyncResp->res, "CredentialBootstrapping/Enabled", enabled,
            "CredentialBootstrapping/EnableAfterReset", enableAfterReset,
            "CredentialBootstrapping/RoleId", roleId))
    {
        return;
    }

    if (enabled)
    {
        setDbusProperty(
            asyncResp, "CredentialBootstrapping/Enabled",
            "xyz.openbmc_project.User.Manager",
            sdbusplus::message::object_path("/xyz/openbmc_project/user"),
            "xyz.openbmc_project.HostInterface.CredentialBootstrapping",
            "CredentialEnabled", *enabled);
    }

    if (enableAfterReset)
    {
        setDbusProperty(
            asyncResp, "CredentialBootstrapping/EnableAfterReset",
            "xyz.openbmc_project.User.Manager",
            sdbusplus::message::object_path("/xyz/openbmc_project/user"),
            "xyz.openbmc_project.HostInterface.CredentialBootstrapping",
            "EnableAfterReset", *enableAfterReset);
    }

    if (roleId)
    {
        std::string sRoleId = getCredentialRoleIdFromUserRoleId(*roleId);
        setDbusProperty(
            asyncResp, "CredentialBootstrapping/RoleId",
            "xyz.openbmc_project.User.Manager",
            sdbusplus::message::object_path("/xyz/openbmc_project/user"),
            "xyz.openbmc_project.HostInterface.CredentialBootstrapping",
            "RoleId", sRoleId);
    }
}

inline void requestRoutesHostInterface(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/HostInterfaces/<str>/")
        .privileges(redfish::privileges::getHostInterface)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleHostInterfaceGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/HostInterfaces/")
        .privileges(redfish::privileges::getHostInterfaceCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleHostInterfaceCollection, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/HostInterfaces/<str>/")
        .privileges(redfish::privileges::patchHostInterface)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleHostInterfacePatch, std::ref(app)));
}

} // namespace redfish
