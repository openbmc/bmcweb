#pragma once

#include <crow/app.h>

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <fstream>

namespace crow {
namespace redfish {

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  
  // noop for now
  return;


  CROW_ROUTE(app, "/redfish/").methods("GET"_method)([]() {
    return nlohmann::json{{"v1", "/redfish/v1/"}};
  });
  CROW_ROUTE(app, "/redfish/v1/").methods("GET"_method)([]() {
    return nlohmann::json{
        {"@odata.context", "/redfish/v1/$metadata#ServiceRoot.ServiceRoot"},
        {"@odata.id", "/redfish/v1/"},
        {"@odata.type", "#ServiceRoot.v1_1_1.ServiceRoot"},
        {"Id", "RootService"},
        {"Name", "Root Service"},
        {"RedfishVersion", "1.1.0"},
        {"UUID", "bdfc5c6d-07a9-4e67-972f-bd2b30c6a0e8"},
        //{"Systems", {{"@odata.id", "/redfish/v1/Systems"}}},
        //{"Chassis", {{"@odata.id", "/redfish/v1/Chassis"}}},
        //{"Managers", {{"@odata.id", "/redfish/v1/Managers"}}},
        //{"SessionService", {{"@odata.id", "/redfish/v1/SessionService"}}},
        {"AccountService", {{"@odata.id", "/redfish/v1/AccountService"}}},
        //{"UpdateService", {{"@odata.id", "/redfish/v1/UpdateService"}}},
        /*{"Links",
         {{"Sessions",
           {{"@odata.id", "/redfish/v1/SessionService/Sessions"}}}}}*/
    };
  });

  CROW_ROUTE(app, "/redfish/v1/Chassis").methods("GET"_method)([]() {
    std::vector<std::string> entities;
    std::ifstream f("~/system.json");
    nlohmann::json input;
    input << f;
    for (auto it = input.begin(); it != input.end(); it++) {
      auto value = it.value();
      if (value["type"] == "Chassis") {
         std::string str = value["name"];
         entities.emplace_back(str);
      }
    }
    auto ret = nlohmann::json{
        {"@odata.context",
         "/redfish/v1/$metadata#ChassisCollection.ChassisCollection"},
        {"@odata.id", "/redfish/v1/Chassis"},
        {"@odata.type", "#ChassisCollection.ChassisCollection"},
        {"Name", "Chassis Collection"},
        {"Members@odata.count", entities.size()}};
    return ret;
  });

  CROW_ROUTE(app, "/redfish/v1/AccountService").methods("GET"_method)([]() {
    return nlohmann::json{
        {"@odata.context",
         "/redfish/v1/$metadata#AccountService.AccountService"},
        {"@odata.id", "/redfish/v1/AccountService"},
        {"@odata.type", "#AccountService.v1_1_0.AccountService"},
        {"Id", "AccountService"},
        {"Name", "Account Service"},
        {"Description", "BMC User Accounts"},
        {"Status",
         // TODO(ed) health rollup
         {{"State", "Enabled"}, {"Health", "OK"}, {"HealthRollup", "OK"}}},
        {"ServiceEnabled", true},
        {"MinPasswordLength", 1},
        {"MaxPasswordLength", 20},
        {"Accounts", {{"@odata.id", "/redfish/v1/AccountService/Accounts"}}},
        //{"Roles", {{"@odata.id", "/redfish/v1/AccountService/Roles"}}}
    };
  });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Accounts")
      .methods("GET"_method)([](const crow::request& req, crow::response& res) {
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
                nlohmann::json return_json{
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
                nlohmann::json member_array;
                int user_index = 0;
                for (auto& user : users) {
                  member_array.push_back(
                      {{"@odata.id", "/redfish/v1/AccountService/Accounts/" +
                                         std::to_string(++user_index)}});
                }
                return_json["Members"] = member_array;
              }
              res.end();
            }, user_list);
      });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Accounts/<int>/")
      .methods("GET"_method)([](int account_index) {
        return nlohmann::json{
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
      });
}
}  // namespace redfish
}  // namespace crow
