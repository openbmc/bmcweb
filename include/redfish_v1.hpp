#pragma once

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
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

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/redfish/")
      .methods("GET"_method)([](const crow::request& req, crow::response& res) {
        res.json_value = {{"v1", "/redfish/v1/"}};
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/Chassis/")
      .methods("GET"_method)(
          [&](const crow::request& req, crow::response& res) {
            std::vector<std::string> entities;
            /*std::ifstream f("~/system.json");

            nlohmann::json input = nlohmann::json::parse(f);
            for (auto it = input.begin(); it != input.end(); it++) {
              auto value = it.value();
              if (value["type"] == "Chassis") {
                std::string str = value["name"];
                entities.emplace_back(str);
              }
            }
            */

            res.json_value = {
                {"@odata.context",
                 "/redfish/v1/$metadata#ChassisCollection.ChassisCollection"},
                {"@odata.id", "/redfish/v1/Chassis"},
                {"@odata.type", "#ChassisCollection.ChassisCollection"},
                {"Name", "Chassis Collection"},
                {"Members@odata.count", entities.size()}};

            get_redfish_sub_routes(app, "/redfish/v1/Chassis", res.json_value);
            res.end();
          });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        boost::asio::io_service io;
        auto bus = std::make_shared<dbus::connection>(io, dbus::bus::session);
        dbus::endpoint user_list("org.openbmc.UserManager",
                                 "/org/openbmc/UserManager/Users",
                                 "org.openbmc.Enrol", "UserList");
        bus->async_method_call(
            [&](const boost::system::error_code ec,
                const std::vector<std::string>& users) {
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
                for (int user_index = 0; user_index < users.size();
                     user_index++) {
                  member_array.push_back(
                      {{"@odata.id", "/redfish/v1/AccountService/Accounts/" +
                                         std::to_string(user_index)}});
                }
                res.json_value["Members"] = member_array;
              }
              res.end();
            },
            user_list);
      });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Accounts/<int>/")
      .methods("GET"_method)([](const crow::request& req, crow::response& res,
                                int account_index) {
        res.json_value = {
            {"@odata.context",
             "/redfish/v1/$metadata#ManagerAccount.ManagerAccount"},
            {"@odata.id", "/redfish/v1/AccountService/Accounts/1"},
            {"@odata.type", "#ManagerAccount.v1_0_3.ManagerAccount"},
            {"Id", "1"},
            {"Name", "User Account"},
            {"Description", "User Account"},
            {"Enabled", false},
            {"Password", nullptr},
            {"UserName", "anonymous"},
            {"RoleId", "NoAccess"},
            {"Links",
             {{"Role",
               {{"@odata.id", "/redfish/v1/AccountService/Roles/NoAccess"}}}}}};
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/Managers/")
      .methods("GET"_method)(
          [&](const crow::request& req, crow::response& res) {
            res.json_value = {
                {"@odata.context",
                 "/redfish/v1/$metadata#ManagerCollection.ManagerCollection"},
                {"@odata.id", "/redfish/v1/Managers"},
                {"@odata.type", "#ManagerCollection.ManagerCollection"},
                {"Name", "Manager Collection"},
                {"Members@odata.count", 1},
                {"Members", {{{"@odata.id", "/redfish/v1/Managers/openbmc"}}}}};
            res.end();
          });

  CROW_ROUTE(app, "/redfish/v1/Managers/openbmc/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        time_t t = time(NULL);
        tm* mytime = std::localtime(&t);
        if (mytime == nullptr) {
          res.code = 500;
          res.end();
          return;
        }
        std::array<char, 100> time_buffer;
        std::size_t len = std::strftime(time_buffer.data(), time_buffer.size(),
                                        "%FT%TZ", mytime);
        if (len == 0) {
          res.code = 500;
          res.end();
          return;
        }
        res.json_value = {
            {"@odata.context", "/redfish/v1/$metadata#Manager.Manager"},
            {"@odata.id", "/redfish/v1/Managers/openbmc"},
            {"@odata.type", "#Manager.v1_3_0.Manager"},
            {"Id", "openbmc"},
            {"Name", "OpenBmc Manager"},
            {"Description", "Baseboard Management Controller"},
            {"UUID", app.template get_middleware<PersistentData::Middleware>()
                         .system_uuid},
            {"Model", "OpenBmc"},  // TODO(ed), get model
            {"DateTime", time_buffer.data()},
            {"Status",
             {{"State", "Enabled"}, {"Health", "OK"}, {"HealthRollup", "OK"}}},
            {"FirmwareVersion", "1234456789"},  // TODO(ed) get fwversion
            {"PowerState", "On"}};
        get_redfish_sub_routes(app, "/redfish/v1/Managers/openbmc/",
                               res.json_value);
        res.end();
      });

}
}  // namespace redfish
}  // namespace crow
