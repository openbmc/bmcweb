#pragma once

#include <crow/app.h>

#include <boost/algorithm/string.hpp>
#include <dbus_singleton.hpp>
#include <fstream>
#include <persistent_data_middleware.hpp>
#include <streambuf>
#include <string>
#include <token_authorization_middleware.hpp>
namespace crow
{
namespace redfish
{

using ManagedObjectType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<
                         std::string, sdbusplus::message::variant<bool>>>>>;

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/redfish/")
        .methods("GET"_method)(
            [](const crow::Request& req, crow::Response& res) {
                res.jsonValue = {{"v1", "/redfish/v1/"}};
                res.end();
            });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
        .methods(
            "GET"_method)([&](const crow::Request& req, crow::Response& res) {
            crow::connections::systemBus->async_method_call(
                [&](const boost::system::error_code ec,
                    const ManagedObjectType& users) {
                    if (ec)
                    {
                        res.result(
                            boost::beast::http::status::internal_server_error);
                    }
                    else
                    {
                        res.jsonValue = {
                            {"@odata.context",
                             "/redfish/v1/"
                             "$metadata#ManagerAccountCollection."
                             "ManagerAccountCollection"},
                            {"@odata.id",
                             "/redfish/v1/AccountService/Accounts"},
                            {"@odata.type", "#ManagerAccountCollection."
                                            "ManagerAccountCollection"},
                            {"Name", "Accounts Collection"},
                            {"Description", "BMC User Accounts"},
                            {"Members@odata.count", users.size()}};
                        nlohmann::json memberArray = nlohmann::json::array();
                        int userIndex = 0;
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
                                {{"@odata.id",
                                  "/redfish/v1/AccountService/Accounts/" +
                                      path.substr(lastIndex)}});
                        }
                        res.jsonValue["Members"] = memberArray;
                    }
                    res.end();
                },
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        .methods("GET"_method)([](const crow::Request& req, crow::Response& res,
                                  const std::string& account_name) {
            crow::connections::systemBus->async_method_call(
                [&, accountName{std::move(account_name)}](
                    const boost::system::error_code ec,
                    const ManagedObjectType& users) {
                    if (ec)
                    {
                        res.result(
                            boost::beast::http::status::internal_server_error);
                    }
                    else
                    {
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
                                res.jsonValue = {
                                    {"@odata.context",
                                     "/redfish/v1/"
                                     "$metadata#ManagerAccount.ManagerAccount"},
                                    {"@odata.id",
                                     "/redfish/v1/AccountService/Accounts/1"},
                                    {"@odata.type",
                                     "#ManagerAccount.v1_0_3.ManagerAccount"},
                                    {"Id", "1"},
                                    {"Name", "User Account"},
                                    {"Description", "User Account"},
                                    {"Enabled", false},
                                    {"Password", nullptr},
                                    {"UserName", accountName},
                                    {"RoleId", "Administrator"},
                                    {"Links",
                                     {{"Role",
                                       {{"@odata.id",
                                         "/redfish/v1/AccountService/Roles/"
                                         "Administrator"}}}}}};
                                break;
                            }
                        }
                        if (res.jsonValue.is_null())
                        {
                            res.result(boost::beast::http::status::not_found);
                        }
                    }
                    res.end();
                },
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        });
}
} // namespace redfish
} // namespace crow
