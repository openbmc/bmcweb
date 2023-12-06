/*
// Copyright (c) 2025 Ampere Computing
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "app.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

namespace redfish
{

inline void handleHostInterfaceCollection(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (managerId != BMCWEB_REDFISH_MANAGER_URI_NAME)
    {
        BMCWEB_LOG_ERROR("Invalid manager Id. Only support bmc.");
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.context"] =
        "/redfish/v1/$metadata#HostInterfaceCollection.HostInterfaceCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/HostInterfaces", managerId);
    asyncResp->res.jsonValue["@odata.type"] =
        "#HostInterfaceCollection.HostInterfaceCollection";
    asyncResp->res.jsonValue["Name"] = "HostInterface Collection";
    asyncResp->res.jsonValue["Description"] = "Collection of HostInterfaces";
    nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];
    memberArray = nlohmann::json::array();
    nlohmann::json::object_t member;
    member["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/HostInterfaces/bmc", managerId);
    memberArray.emplace_back(std::move(member));
    asyncResp->res.jsonValue["Members@odata.count"] = memberArray.size();
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
        BMCWEB_LOG_ERROR("Invalid manager Id. Only support bmc.");
        messages::internalError(asyncResp->res);
        return;
    }

    if (hostInterfaceId != "bmc")
    {
        BMCWEB_LOG_ERROR("Invalid Host Interface Id. Only support bmc.");
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.jsonValue["@odata.context"] =
        "/redfish/v1/$metadata#HostInterface.HostInterface";
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
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error for Properties{}",
                                     ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }
            const bool* enabledAfterReset = nullptr;
            const bool* enabled = nullptr;
            const std::string* roldId = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), properties,
                "EnableAfterReset", enabledAfterReset, "Enabled", enabled,
                "RoleId", roldId);
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
                asyncResp->res.jsonValue["CredentialBootstrapping"]["RoleId"] =
                    *roldId;
                asyncResp->res.jsonValue["Links"]["CredentialBootstrappingRole"]
                                        ["@odata.id"] = boost::urls::format(
                    "/redfish/v1/AccountService/Roles/{}", *roldId);
            }
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
