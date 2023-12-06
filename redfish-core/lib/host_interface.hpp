// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2025 Ampere Computing
#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

namespace redfish
{

constexpr std::string_view bmcRedfishHostIntfUriName = "bmc";

inline void handleHostInterfaceCollection(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
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
    const std::string* roldId = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "EnableAfterReset",
        enabledAfterReset, "Enabled", enabled, "RoleId", roldId);
    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    if (enabledAfterReset != nullptr)
    {
        asyncResp->res
            .jsonValue["CredentialBootstrapping"]["EnableAfterReset"] =
            *enabledAfterReset;
    }
    if (enabled != nullptr)
    {
        asyncResp->res.jsonValue["CredentialBootstrapping"]["Enabled"] =
            *enabled;
    }
    if (roldId != nullptr)
    {
        asyncResp->res.jsonValue["CredentialBootstrapping"]["RoleId"] = *roldId;
        asyncResp->res
            .jsonValue["Links"]["CredentialBootstrappingRole"]["@odata.id"] =
            boost::urls::format("/redfish/v1/AccountService/Roles/{}", *roldId);
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

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        BMCWEB_LOG_WARNING("Invalid manager Id. Only support bmc.");
        messages::resourceNotFound(asyncResp->res, "Manager", managerId);
        return;
    }

    if (hostInterfaceId != bmcRedfishHostIntfUriName)
    {
        BMCWEB_LOG_WARNING("Invalid Host Interface Id. Only support bmc.");
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
    asyncResp->res.jsonValue["HostInterfaceType"] = "NetworkHostInterface";
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";

    dbus::utility::getAllProperties(
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.HostInterface.CredentialBootstrapping",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
            onGetCredentialBootstrappingProperty(asyncResp, ec, properties);
        });
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
}

} // namespace redfish
