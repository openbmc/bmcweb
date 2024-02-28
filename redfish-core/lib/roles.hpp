/*
// Copyright (c) 2018 Intel Corporation
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
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>

#include <variant>
namespace redfish
{

inline std::string getRoleFromPrivileges(std::string_view priv)
{
    if (priv == "priv-admin")
    {
        return "Administrator";
    }
    if (priv == "priv-user")
    {
        return "ReadOnly";
    }
    if (priv == "priv-operator")
    {
        return "Operator";
    }
    if (priv == "priv-oemibmserviceagent")
    {
        return "OemIBMServiceAgent";
    }
    return "";
}

inline bool getAssignedPrivFromRole(std::string_view role,
                                    nlohmann::json& privArray)
{
    if ((role == "Administrator") || (role == "OemIBMServiceAgent"))
    {
        privArray = {"Login", "ConfigureManager", "ConfigureUsers",
                     "ConfigureSelf", "ConfigureComponents"};
    }
    else if (role == "Operator")
    {
        privArray = {"Login", "ConfigureSelf", "ConfigureComponents"};
    }
    else if (role == "ReadOnly")
    {
        privArray = {"Login", "ConfigureSelf"};
    }
    else
    {
        return false;
    }
    return true;
}

inline bool getOemPrivFromRole(std::string_view role, nlohmann::json& privArray)
{
    if ((role == "Administrator") || (role == "Operator") ||
        (role == "ReadOnly") || (role == "NoAccess"))
    {
        privArray = nlohmann::json::array();
    }
    else if (role == "OemIBMServiceAgent")
    {
        privArray = {"OemIBMPerformService"};
    }
    else
    {
        return false;
    }
    return true;
}

inline bool isRestrictedRole(const std::string& role)
{
    return role == "OemIBMServiceAgent";
}

inline void requestRoutesRoles(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Roles/<str>/")
        .privileges(redfish::privileges::getRole)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& roleId) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        nlohmann::json privArray = nlohmann::json::array();
        if (!getAssignedPrivFromRole(roleId, privArray))
        {
            messages::resourceNotFound(asyncResp->res, "Role", roleId);

            return;
        }

        nlohmann::json oemPrivArray = nlohmann::json::array();
        if (!getOemPrivFromRole(roleId, oemPrivArray))
        {
            messages::resourceNotFound(asyncResp->res, "Role", roleId);
            return;
        }

        asyncResp->res.jsonValue["@odata.type"] = "#Role.v1_3_0.Role";
        asyncResp->res.jsonValue["Name"] = "User Role";
        asyncResp->res.jsonValue["OemPrivileges"] = std::move(oemPrivArray);
        asyncResp->res.jsonValue["IsPredefined"] = true;
        asyncResp->res.jsonValue["Id"] = roleId;
        asyncResp->res.jsonValue["RoleId"] = roleId;
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/AccountService/Roles/{}", roleId);
        asyncResp->res.jsonValue["AssignedPrivileges"] = std::move(privArray);
        asyncResp->res.jsonValue["Restricted"] = isRestrictedRole(roleId);
        if (roleId == "OemIBMServiceAgent")
        {
            asyncResp->res.jsonValue["Description"] = "ServiceAgent";
        }
        else
        {
            asyncResp->res.jsonValue["Description"] = roleId;
        }
    });
}

inline void requestRoutesRoleCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Roles/")
        .privileges(redfish::privileges::getRoleCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/AccountService/Roles";
        asyncResp->res.jsonValue["@odata.type"] =
            "#RoleCollection.RoleCollection";
        asyncResp->res.jsonValue["Name"] = "Roles Collection";
        asyncResp->res.jsonValue["Description"] = "BMC User Roles";

        sdbusplus::asio::getProperty<std::vector<std::string>>(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            "/xyz/openbmc_project/user", "xyz.openbmc_project.User.Manager",
            "AllPrivileges",
            [asyncResp](const boost::system::error_code& ec,
                        const std::vector<std::string>& privList) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];
            memberArray = nlohmann::json::array();
            for (const std::string& priv : privList)
            {
                std::string role = getRoleFromPrivileges(priv);
                if (!role.empty())
                {
                    nlohmann::json::object_t member;
                    member["@odata.id"] = boost::urls::format(
                        "/redfish/v1/AccountService/Roles/{}", role);
                    memberArray.emplace_back(std::move(member));
                }
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                memberArray.size();
        });
    });
}

} // namespace redfish
