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
        Node::json["@odata.id"] = "/redfish/v1/AccountService";
        Node::json["@odata.type"] = "#AccountService.v1_1_0.AccountService";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#AccountService.AccountService";
        Node::json["Id"] = "AccountService";
        Node::json["Description"] = "BMC User Accounts";
        Node::json["Name"] = "Account Service";
        Node::json["ServiceEnabled"] = true;
        Node::json["MinPasswordLength"] = 1;
        Node::json["MaxPasswordLength"] = 20;
        Node::json["Accounts"]["@odata.id"] =
            "/redfish/v1/AccountService/Accounts";
        Node::json["Roles"]["@odata.id"] = "/redfish/v1/AccountService/Roles";

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
        res.jsonValue = Node::json;
        res.end();
    }
};
class AccountsCollection : public Node
{
  public:
    AccountsCollection(CrowApp& app) :
        Node(app, "/redfish/v1/AccountService/Accounts/")
    {

        Node::json = {{"@odata.context", "/redfish/v1/"
                                         "$metadata#ManagerAccountCollection."
                                         "ManagerAccountCollection"},
                      {"@odata.id", "/redfish/v1/AccountService/Accounts"},
                      {"@odata.type", "#ManagerAccountCollection."
                                      "ManagerAccountCollection"},
                      {"Name", "Accounts Collection"},
                      {"Description", "BMC User Accounts"}};

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
        res.jsonValue = Node::json;
        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const ManagedObjectType& users) {
                if (ec)
                {
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
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

        nlohmann::json patchRequest;
        if (!json_util::processJsonFromRequest(res, req, patchRequest))
        {
            return;
        }

        const std::string* username = nullptr;
        const std::string* password = nullptr;
        // Default to user
        std::string privilege = "priv-user";
        // default to enabled
        bool enabled = true;
        for (const auto& item : patchRequest.items())
        {
            if (item.key() == "UserName")
            {
                username = item.value().get_ptr<const std::string*>();
                if (username == nullptr)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::propertyValueFormatError(item.value().dump(),
                                                           item.key()));
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }
            }
            else if (item.key() == "Enabled")
            {
                const bool* enabledJson = item.value().get_ptr<const bool*>();
                if (enabledJson == nullptr)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::propertyValueFormatError(item.value().dump(),
                                                           item.key()));
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }
                enabled = *enabledJson;
            }
            else if (item.key() == "Password")
            {
                password = item.value().get_ptr<const std::string*>();
                if (password == nullptr)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::propertyValueFormatError(item.value().dump(),
                                                           item.key()));
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }
            }
            else if (item.key() == "RoleId")
            {
                const std::string* roleIdJson =
                    item.value().get_ptr<const std::string*>();
                if (roleIdJson == nullptr)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::propertyValueFormatError(item.value().dump(),
                                                           item.key()));
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }
                const char* priv = getRoleIdFromPrivilege(*roleIdJson);
                if (priv == nullptr)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::propertyValueNotInList(*roleIdJson,
                                                         item.key()));
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }
                privilege = priv;
            }
            else
            {
                messages::addMessageToErrorJson(
                    asyncResp->res.jsonValue,
                    messages::propertyNotWritable(item.key()));
                asyncResp->res.result(boost::beast::http::status::bad_request);
                return;
            }
        }

        if (username == nullptr)
        {
            messages::addMessageToErrorJson(
                asyncResp->res.jsonValue,
                messages::createFailedMissingReqProperties("UserName"));
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }

        if (password == nullptr)
        {
            messages::addMessageToErrorJson(
                asyncResp->res.jsonValue,
                messages::createFailedMissingReqProperties("Password"));
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, username{std::string(*username)},
             password{std::string(*password)}](
                const boost::system::error_code ec) {
                if (ec)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::resourceAlreadyExists(
                            "#ManagerAccount.v1_0_3.ManagerAccount", "UserName",
                            username));
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
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
                                asyncResp->res.result(
                                    boost::beast::http::status::
                                        internal_server_error);
                                return;
                            }

                            asyncResp->res.result(
                                boost::beast::http::status::bad_request);
                        },
                        "xyz.openbmc_project.User.Manager",
                        "/xyz/openbmc_project/user/" + username,
                        "xyz.openbmc_project.Object.Delete", "Delete");

                    BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                    return;
                }

                messages::addMessageToJsonRoot(asyncResp->res.jsonValue,
                                               messages::created());
                asyncResp->res.addHeader(
                    "Location",
                    "/redfish/v1/AccountService/Accounts/" + username);
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.Manager", "CreateUser", *username,
            std::array<const char*, 4>{"ipmi", "redfish", "ssh", "web"},
            privilege, enabled);
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
        Node::json = {{"@odata.context",
                       "/redfish/v1/$metadata#ManagerAccount.ManagerAccount"},
                      {"@odata.type", "#ManagerAccount.v1_0_3.ManagerAccount"},

                      {"Name", "User Account"},
                      {"Description", "User Account"},
                      {"Password", nullptr},
                      {"RoleId", "Administrator"},
                      {"Links",
                       {{"Role",
                         {{"@odata.id", "/redfish/v1/AccountService/Roles/"
                                        "Administrator"}}}}}};

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
        res.jsonValue = Node::json;
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, accountName{std::string(params[0])}](
                const boost::system::error_code ec,
                const ManagedObjectType& users) {
                if (ec)
                {
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
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
                                            mapbox::getPtr<const bool>(
                                                property.second);
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
                                            mapbox::getPtr<const bool>(
                                                property.second);
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

                asyncResp->res.result(boost::beast::http::status::not_found);
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
            res.result(boost::beast::http::status::internal_server_error);
            return;
        }

        nlohmann::json patchRequest;
        if (!json_util::processJsonFromRequest(res, req, patchRequest))
        {
            return;
        }

        // Check the user exists before updating the fields
        checkDbusPathExists(
            "/xyz/openbmc_project/users/" + params[0],
            [username{std::string(params[0])},
             patchRequest(std::move(patchRequest)),
             asyncResp](bool userExists) {
                if (!userExists)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::resourceNotFound(
                            "#ManagerAccount.v1_0_3.ManagerAccount", username));

                    asyncResp->res.result(
                        boost::beast::http::status::not_found);
                    return;
                }

                for (const auto& item : patchRequest.items())
                {
                    if (item.key() == "Password")
                    {
                        const std::string* passStr =
                            item.value().get_ptr<const std::string*>();
                        if (passStr == nullptr)
                        {
                            messages::addMessageToErrorJson(
                                asyncResp->res.jsonValue,
                                messages::propertyValueFormatError(
                                    item.value().dump(), "Password"));
                            return;
                        }
                        BMCWEB_LOG_DEBUG << "Updating user: " << username
                                         << " to password " << *passStr;
                        if (!pamUpdatePassword(username, *passStr))
                        {
                            BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                            asyncResp->res.result(boost::beast::http::status::
                                                      internal_server_error);
                            return;
                        }
                    }
                    else if (item.key() == "Enabled")
                    {
                        const bool* enabledBool =
                            item.value().get_ptr<const bool*>();

                        if (enabledBool == nullptr)
                        {
                            messages::addMessageToErrorJson(
                                asyncResp->res.jsonValue,
                                messages::propertyValueFormatError(
                                    item.value().dump(), "Enabled"));
                            return;
                        }
                        crow::connections::systemBus->async_method_call(
                            [asyncResp](const boost::system::error_code ec) {
                                if (ec)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "D-Bus responses error: " << ec;
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
                                    return;
                                }
                                // TODO Consider support polling mechanism to
                                // verify status of host and chassis after
                                // execute the requested action.
                                BMCWEB_LOG_DEBUG << "Response with no content";
                                asyncResp->res.result(
                                    boost::beast::http::status::no_content);
                            },
                            "xyz.openbmc_project.User.Manager",
                            "/xyz/openbmc_project/users/" + username,
                            "org.freedesktop.DBus.Properties", "Set",
                            "xyz.openbmc_project.User.Attributes"
                            "UserEnabled",
                            sdbusplus::message::variant<bool>{*enabledBool});
                    }
                    else
                    {
                        messages::addMessageToErrorJson(
                            asyncResp->res.jsonValue,
                            messages::propertyNotWritable(item.key()));
                        asyncResp->res.result(
                            boost::beast::http::status::bad_request);
                        return;
                    }
                }
            });
    }

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            return;
        }

        const std::string userPath = "/xyz/openbmc_project/user/" + params[0];

        crow::connections::systemBus->async_method_call(
            [asyncResp, username{std::move(params[0])}](
                const boost::system::error_code ec) {
                if (ec)
                {
                    messages::addMessageToErrorJson(
                        asyncResp->res.jsonValue,
                        messages::resourceNotFound(
                            "#ManagerAccount.v1_0_3.ManagerAccount", username));
                    asyncResp->res.result(
                        boost::beast::http::status::not_found);
                    return;
                }

                messages::addMessageToJsonRoot(asyncResp->res.jsonValue,
                                               messages::accountRemoved());
            },
            "xyz.openbmc_project.User.Manager", userPath,
            "xyz.openbmc_project.Object.Delete", "Delete");
    }
};

} // namespace redfish
