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

template <typename... Middlewares>
void get_redfish_sub_routes(Crow<Middlewares...>& app, const std::string& url,
                            nlohmann::json& j) {
  std::vector<const std::string*> routes = app.get_routes(url);
  for (auto route : routes) {
    auto redfish_sub_route =
        route->substr(url.size(), route->size() - url.size() - 1);
    // check if the route is at this level, and we didn't find and exact match
    // also, filter out resources that start with $ to remove $metadata
    if (!redfish_sub_route.empty() && redfish_sub_route[0] != '$' &&
        redfish_sub_route.find('/') == std::string::npos) {
      j[redfish_sub_route] = nlohmann::json{{"@odata.id", *route}};
    }
  }
}

std::string execute_process(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
      result += buffer.data();
  }
  return result;
}

// GetManagedObjects unpack type.  Observe that variant has only one bool type,
// because we don't actually use the values it provides
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
                res.code = 500;
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
                      static_cast<std::string>(user.first);
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
                res.code = 500;
              } else {
                for (auto& user : users) {
                  const std::string& path =
                      static_cast<std::string>(user.first);
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
                  res.code = 404;
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
