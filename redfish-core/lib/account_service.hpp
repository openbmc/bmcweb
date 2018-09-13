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

#include <openbmc_dbus_rest.hpp>

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
};

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
                      {"Enabled", false},
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
};

} // namespace redfish
