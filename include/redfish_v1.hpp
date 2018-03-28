#pragma once

#include <dbus_singleton.hpp>
#include <persistent_data_middleware.hpp>
#include <token_authorization_middleware.hpp>
#include <fstream>
#include <streambuf>
#include <string>
#include <crow/app.h>
#include <boost/algorithm/string.hpp>
namespace crow {
namespace redfish {

using ManagedObjectType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<
                         std::string, sdbusplus::message::variant<bool>>>>>;

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/redfish/")
      .methods("GET"_method)([](const crow::request& req, crow::response& res) {
        res.json_value = {{"v1", "/redfish/v1/"}};
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        crow::connections::system_bus->async_method_call(
            [&](const boost::system::error_code ec,
                const ManagedObjectType& users) {
              if (ec) {
                res.result(boost::beast::http::status::internal_server_error);
              } else {
                res.json_value = {
                    {"@odata.context",
                     "/redfish/v1/"
                     "$metadata#ManagerAccountCollection."
                     "ManagerAccountCollection"},
                    {"@odata.id", "/redfish/v1/AccountService/Accounts"},
                    {"@odata.type",
                     "#ManagerAccountCollection.ManagerAccountCollection"},
                    {"Name", "Accounts Collection"},
                    {"Description", "BMC User Accounts"},
                    {"Members@odata.count", users.size()}};
                nlohmann::json member_array = nlohmann::json::array();
                int user_index = 0;
                for (auto& user : users) {
                  const std::string& path =
                      static_cast<const std::string&>(user.first);
                  std::size_t last_index = path.rfind("/");
                  if (last_index == std::string::npos) {
                    last_index = 0;
                  } else {
                    last_index += 1;
                  }
                  member_array.push_back(
                      {{"@odata.id", "/redfish/v1/AccountService/Accounts/" +
                                         path.substr(last_index)}});
                }
                res.json_value["Members"] = member_array;
              }
              res.end();
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
      });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
      .methods("GET"_method)([](const crow::request& req, crow::response& res,
                                const std::string& account_name) {

        crow::connections::system_bus->async_method_call(
            [&, account_name{std::move(account_name)} ](
                const boost::system::error_code ec,
                const ManagedObjectType& users) {
              if (ec) {
                res.result(boost::beast::http::status::internal_server_error);
              } else {
                for (auto& user : users) {
                  const std::string& path =
                      static_cast<const std::string&>(user.first);
                  std::size_t last_index = path.rfind("/");
                  if (last_index == std::string::npos) {
                    last_index = 0;
                  } else {
                    last_index += 1;
                  }
                  if (path.substr(last_index) == account_name) {
                    res.json_value = {
                        {"@odata.context",
                         "/redfish/v1/$metadata#ManagerAccount.ManagerAccount"},
                        {"@odata.id", "/redfish/v1/AccountService/Accounts/1"},
                        {"@odata.type",
                         "#ManagerAccount.v1_0_3.ManagerAccount"},
                        {"Id", "1"},
                        {"Name", "User Account"},
                        {"Description", "User Account"},
                        {"Enabled", false},
                        {"Password", nullptr},
                        {"UserName", account_name},
                        {"RoleId", "Administrator"},
                        {"Links",
                         {{"Role",
                           {{"@odata.id",
                             "/redfish/v1/AccountService/Roles/"
                             "Administrator"}}}}}};
                    break;
                  }
                }
                if (res.json_value.is_null()) {
                  res.result(boost::beast::http::status::not_found);
                }
              }
              res.end();
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
      });
}
}  // namespace redfish
}  // namespace crow
