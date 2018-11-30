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

#include <error_messages.hpp>
#include <openbmc_dbus_rest.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

using ManagedObjectType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<
                         std::string, sdbusplus::message::variant<bool>>>>>;

class AccountService : public Node
{
  public:
    AccountService(CrowApp& app) : Node(app, "/redfish/v1/AccountService/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get,
             {{"ConfigureUsers"}, {"ConfigureManager"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::put, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::post, {{"ConfigureUsers"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue["@odata.id"] = "/redfish/v1/AccountService";
        res.jsonValue["@odata.type"] = "#AccountService.v1_1_0.AccountService";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#AccountService.AccountService";
        res.jsonValue["Id"] = "AccountService";
        res.jsonValue["Description"] = "BMC User Accounts";
        res.jsonValue["Name"] = "Account Service";
        res.jsonValue["ServiceEnabled"] = true;
        res.jsonValue["MinPasswordLength"] = 1;
        res.jsonValue["MaxPasswordLength"] = 20;
        res.jsonValue["Accounts"]["@odata.id"] =
            "/redfish/v1/AccountService/Accounts";
        res.jsonValue["Roles"]["@odata.id"] =
            "/redfish/v1/AccountService/Roles";

        res.end();
    }
};
class AccountsCollection : public Node
{
  public:
    AccountsCollection(CrowApp& app) :
        Node(app, "/redfish/v1/AccountService/Accounts/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get,
             {{"ConfigureUsers"}, {"ConfigureManager"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::put, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::post, {{"ConfigureUsers"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue = {{"@odata.context",
                          "/redfish/v1/"
                          "$metadata#ManagerAccountCollection."
                          "ManagerAccountCollection"},
                         {"@odata.id", "/redfish/v1/AccountService/Accounts"},
                         {"@odata.type", "#ManagerAccountCollection."
                                         "ManagerAccountCollection"},
                         {"Name", "Accounts Collection"},
                         {"Description", "BMC User Accounts"}};

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const ManagedObjectType& users) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                nlohmann::json& memberArray =
                    asyncResp->res.jsonValue["Members"];
                memberArray = nlohmann::json::array();

                asyncResp->res.jsonValue["Members@odata.count"] = users.size();
                for (auto& user : users)
                {
                    const std::string& path =
                        static_cast<const std::string&>(user.first);
                    std::size_t lastIndex = path.rfind("/");
                    if (lastIndex == std::string::npos)
                    {
                        lastIndex = 0;
                    }
                    else
                    {
                        lastIndex += 1;
                    }
                    memberArray.push_back(
                        {{"@odata.id", "/redfish/v1/AccountService/Accounts/" +
                                           path.substr(lastIndex)}});
                }
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        std::string username;
        std::string password;
        boost::optional<std::string> roleId("User");
        boost::optional<bool> enabled = true;
        if (!json_util::readJson(req, res, "UserName", username, "Password",
                                 password, "RoleId", roleId, "Enabled",
                                 enabled))
        {
            return;
        }

        const char* priv = getRoleIdFromPrivilege(*roleId);
        if (priv == nullptr)
        {
            messages::propertyValueNotInList(asyncResp->res, *roleId, "RoleId");
            return;
        }
        roleId = priv;

        crow::connections::systemBus->async_method_call(
            [asyncResp, username, password{std::move(password)}](
                const boost::system::error_code ec) {
                if (ec)
                {
                    messages::resourceAlreadyExists(
                        asyncResp->res, "#ManagerAccount.v1_0_3.ManagerAccount",
                        "UserName", username);
                    return;
                }

                if (!pamUpdatePassword(username, password))
                {
                    // At this point we have a user that's been created, but the
                    // password set failed.  Something is wrong, so delete the
                    // user that we've already created
                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            messages::invalidObject(asyncResp->res, "Password");
                        },
                        "xyz.openbmc_project.User.Manager",
                        "/xyz/openbmc_project/user/" + username,
                        "xyz.openbmc_project.Object.Delete", "Delete");

                    BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                    return;
                }

                messages::created(asyncResp->res);
                asyncResp->res.addHeader(
                    "Location",
                    "/redfish/v1/AccountService/Accounts/" + username);
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.Manager", "CreateUser", username,
            std::array<const char*, 4>{"ipmi", "redfish", "ssh", "web"},
            *roleId, *enabled);
    }

    static const char* getRoleIdFromPrivilege(boost::beast::string_view role)
    {
        if (role == "Administrator")
        {
            return "priv-admin";
        }
        else if (role == "Callback")
        {
            return "priv-callback";
        }
        else if (role == "User")
        {
            return "priv-user";
        }
        else if (role == "Operator")
        {
            return "priv-operator";
        }
        return nullptr;
    }
};

template <typename Callback>
inline void checkDbusPathExists(const std::string& path, Callback&& callback)
{
    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code ec,
                                        const GetObjectType& object_names) {
            callback(ec || object_names.size() == 0);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

class ManagerAccount : public Node
{
  public:
    ManagerAccount(CrowApp& app) :
        Node(app, "/redfish/v1/AccountService/Accounts/<str>/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get,
             {{"ConfigureUsers"}, {"ConfigureManager"}, {"ConfigureSelf"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::put, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureUsers"}}},
            {boost::beast::http::verb::post, {{"ConfigureUsers"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        res.jsonValue = {
            {"@odata.context",
             "/redfish/v1/$metadata#ManagerAccount.ManagerAccount"},
            {"@odata.type", "#ManagerAccount.v1_0_3.ManagerAccount"},

            {"Name", "User Account"},
            {"Description", "User Account"},
            {"Password", nullptr},
            {"RoleId", "Administrator"},
            {"Links",
             {{"Role",
               {{"@odata.id",
                 "/redfish/v1/AccountService/Roles/Administrator"}}}}}};

        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, accountName{std::string(params[0])}](
                const boost::system::error_code ec,
                const ManagedObjectType& users) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (auto& user : users)
                {
                    const std::string& path =
                        static_cast<const std::string&>(user.first);
                    std::size_t lastIndex = path.rfind("/");
                    if (lastIndex == std::string::npos)
                    {
                        lastIndex = 0;
                    }
                    else
                    {
                        lastIndex += 1;
                    }
                    if (path.substr(lastIndex) == accountName)
                    {
                        for (const auto& interface : user.second)
                        {
                            if (interface.first ==
                                "xyz.openbmc_project.User.Attributes")
                            {
                                for (const auto& property : interface.second)
                                {
                                    if (property.first == "UserEnabled")
                                    {
                                        const bool* userEnabled =
                                            sdbusplus::message::variant_ns::
                                                get_if<bool>(&property.second);
                                        if (userEnabled == nullptr)
                                        {
                                            BMCWEB_LOG_ERROR
                                                << "UserEnabled wasn't a bool";
                                            continue;
                                        }
                                        asyncResp->res.jsonValue["Enabled"] =
                                            *userEnabled;
                                    }
                                    else if (property.first ==
                                             "UserLockedForFailedAttempt")
                                    {
                                        const bool* userLocked =
                                            sdbusplus::message::variant_ns::
                                                get_if<bool>(&property.second);
                                        if (userLocked == nullptr)
                                        {
                                            BMCWEB_LOG_ERROR
                                                << "UserEnabled wasn't a bool";
                                            continue;
                                        }
                                        asyncResp->res.jsonValue["Locked"] =
                                            *userLocked;
                                    }
                                }
                            }
                        }

                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/AccountService/Accounts/" +
                            accountName;
                        asyncResp->res.jsonValue["Id"] = accountName;
                        asyncResp->res.jsonValue["UserName"] = accountName;

                        return;
                    }
                }

                messages::resourceNotFound(asyncResp->res, "ManagerAccount",
                                           accountName);
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        boost::optional<std::string> password;
        boost::optional<bool> enabled;
        if (!json_util::readJson(req, res, "Password", password, "Enabled",
                                 enabled))
        {
            return;
        }

        // Check the user exists before updating the fields
        checkDbusPathExists(
            "/xyz/openbmc_project/users/" + params[0],
            [username{std::string(params[0])}, password(std::move(password)),
             enabled(std::move(enabled)), asyncResp](bool userExists) {
                if (!userExists)
                {
                    messages::resourceNotFound(
                        asyncResp->res, "#ManagerAccount.v1_0_3.ManagerAccount",
                        username);
                    return;
                }

                if (password)
                {
                    if (!pamUpdatePassword(username, *password))
                    {
                        BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                        messages::internalError(asyncResp->res);
                        return;
                    }
                }

                if (enabled)
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "D-Bus responses error: "
                                                 << ec;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            // TODO Consider support polling mechanism to
                            // verify status of host and chassis after
                            // execute the requested action.
                            messages::success(asyncResp->res);
                        },
                        "xyz.openbmc_project.User.Manager",
                        "/xyz/openbmc_project/users/" + username,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.User.Attributes"
                        "UserEnabled",
                        sdbusplus::message::variant<bool>{*enabled});
                }
            });
    }

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string userPath = "/xyz/openbmc_project/user/" + params[0];

        crow::connections::systemBus->async_method_call(
            [asyncResp, username{std::move(params[0])}](
                const boost::system::error_code ec) {
                if (ec)
                {
                    messages::resourceNotFound(
                        asyncResp->res, "#ManagerAccount.v1_0_3.ManagerAccount",
                        username);
                    return;
                }

                messages::accountRemoved(asyncResp->res);
            },
            "xyz.openbmc_project.User.Manager", userPath,
            "xyz.openbmc_project.Object.Delete", "Delete");
    }
}; // namespace redfish

} // namespace redfish
