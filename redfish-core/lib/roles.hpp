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

#include <app.hpp>
#include <dbus_utility.hpp>
#include <registries/privilege_registry.hpp>
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
    if (priv == "priv-noaccess")
    {
        return "NoAccess";
    }
    return "";
}

inline bool getAssignedPrivFromRole(std::string_view role,
                                    nlohmann::json& privArray)
{
    if (role == "Administrator")
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
    else if (role == "NoAccess")
    {
        privArray = nlohmann::json::array();
    }
    else
    {
        return false;
    }
    return true;
}

inline void requestRoutesRoles(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Roles/<str>/")
        .privileges(redfish::privileges::getRole)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& roleId) {
                nlohmann::json privArray = nlohmann::json::array();
                if (false == getAssignedPrivFromRole(roleId, privArray))
                {
                    messages::resourceNotFound(asyncResp->res, "Role", roleId);

                    return;
                }

                asyncResp->res.jsonValue = {
                    {"@odata.type", "#Role.v1_2_2.Role"},
                    {"Name", "User Role"},
                    {"Description", roleId + " User Role"},
                    {"OemPrivileges", nlohmann::json::array()},
                    {"IsPredefined", true},
                    {"Id", roleId},
                    {"RoleId", roleId},
                    {"@odata.id", "/redfish/v1/AccountService/Roles/" + roleId},
                    {"AssignedPrivileges", std::move(privArray)}};
            });
}

inline void requestRoutesRoleCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Roles/")
        .privileges(redfish::privileges::getRoleCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {
                    {"@odata.id", "/redfish/v1/AccountService/Roles"},
                    {"@odata.type", "#RoleCollection.RoleCollection"},
                    {"Name", "Roles Collection"},
                    {"Description", "BMC User Roles"}};

                sdbusplus::asio::getProperty<std::vector<std::string>>(
                    *crow::connections::systemBus,
                    "xyz.openbmc_project.User.Manager",
                    "/xyz/openbmc_project/user",
                    "xyz.openbmc_project.User.Manager", "AllPrivileges",
                    [asyncResp](const boost::system::error_code ec,
                                const std::vector<std::string>& privList) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        nlohmann::json& memberArray =
                            asyncResp->res.jsonValue["Members"];
                        memberArray = nlohmann::json::array();
                        for (const std::string& priv : privList)
                        {
                            std::string role = getRoleFromPrivileges(priv);
                            if (!role.empty())
                            {
                                memberArray.push_back(
                                    {{"@odata.id",
                                      "/redfish/v1/AccountService/Roles/" +
                                          role}});
                            }
                        }
                        asyncResp->res.jsonValue["Members@odata.count"] =
                            memberArray.size();
                    });
            });
}

} // namespace redfish
