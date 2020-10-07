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

#include "node.hpp"

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

class Roles : public Node
{
  public:
    Roles(App& app) :
        Node(app, "/redfish/v1/AccountService/Roles/<str>/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }
        const std::string& roleId = params[0];
        nlohmann::json privArray = nlohmann::json::array();
        if (false == getAssignedPrivFromRole(roleId, privArray))
        {
            messages::resourceNotFound(res, "Role", roleId);
            res.end();
            return;
        }

        res.jsonValue = {
            {"@odata.type", "#Role.v1_2_2.Role"},
            {"Name", "User Role"},
            {"Description", roleId + " User Role"},
            {"OemPrivileges", nlohmann::json::array()},
            {"IsPredefined", true},
            {"Id", roleId},
            {"RoleId", roleId},
            {"@odata.id", "/redfish/v1/AccountService/Roles/" + roleId},
            {"AssignedPrivileges", std::move(privArray)}};
        res.end();
    }
};

class RoleCollection : public Node
{
  public:
    RoleCollection(App& app) : Node(app, "/redfish/v1/AccountService/Roles/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue = {{"@odata.id", "/redfish/v1/AccountService/Roles"},
                         {"@odata.type", "#RoleCollection.RoleCollection"},
                         {"Name", "Roles Collection"},
                         {"Description", "BMC User Roles"}};

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::variant<std::vector<std::string>>& resp) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                nlohmann::json& memberArray =
                    asyncResp->res.jsonValue["Members"];
                memberArray = nlohmann::json::array();
                const std::vector<std::string>* privList =
                    std::get_if<std::vector<std::string>>(&resp);
                if (privList == nullptr)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                for (const std::string& priv : *privList)
                {
                    std::string role = getRoleFromPrivileges(priv);
                    if (!role.empty())
                    {
                        memberArray.push_back(
                            {{"@odata.id",
                              "/redfish/v1/AccountService/Roles/" + role}});
                    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    memberArray.size();
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.User.Manager", "AllPrivileges");
    }
};

} // namespace redfish
