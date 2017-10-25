#pragma once

#include <crow/app.h>

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

namespace crow {
namespace redfish {

template <typename... Middlewares>
void get_redfish_sub_routes(Crow<Middlewares...>& app, const std::string& url,
                            nlohmann::json& j) {
  auto routes = app.get_routes(url);
  for (auto& route : routes) {
    auto redfish_sub_route =
        route.substr(url.size(), route.size() - url.size() - 1);
    // check if the route is at this level, and we didn't find and exact match
    // also, filter out resources that start with $ to remove $metadata
    if (!redfish_sub_route.empty() && redfish_sub_route[0] != '$' &&
        redfish_sub_route.find('/') == std::string::npos) {
      j[redfish_sub_route] = nlohmann::json{{"@odata.id", route}};
    }
  }
}

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  CROW_ROUTE(app, "/redfish/")
      .methods("GET"_method)([](const crow::request& req, crow::response& res) {
        res.json_value = {{"v1", "/redfish/v1/"}};
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        res.json_value = {
            {"@odata.context", "/redfish/v1/$metadata#ServiceRoot.ServiceRoot"},
            {"@odata.id", "/redfish/v1/"},
            {"@odata.type", "#ServiceRoot.v1_1_1.ServiceRoot"},
            {"Id", "RootService"},
            {"Name", "Root Service"},
            {"RedfishVersion", "1.1.0"},
            {"Links",
             {{"Sessions",
               {{"@odata.id", "/redfish/v1/SessionService/Sessions/"}}}}}};

        res.json_value["UUID"] =
            app.template get_middleware<PersistentData::Middleware>()
                .system_uuid;
        get_redfish_sub_routes(app, "/redfish/v1/", res.json_value);
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

  CROW_ROUTE(app, "/redfish/v1/AccountService/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        res.json_value = {
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
        };
        get_redfish_sub_routes(app, "/redfish/v1/AccountService",
                               res.json_value);
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Roles/")
      .methods("GET"_method)(
          [&](const crow::request& req, crow::response& res) {
            res.json_value = {
                {"@odata.context",
                 "/redfish/v1/$metadata#RoleCollection.RoleCollection"},
                {"@odata.id", "/redfish/v1/AccountService/Roles"},
                {"@odata.type", "#RoleCollection.RoleCollection"},
                {"Name", "Account Service"},
                {"Description", "BMC User Roles"},
                {"Members@odata.count", 1},
                {"Members",
                 {{"@odata.id",
                   "/redfish/v1/AccountService/Roles/Administrator"}}}};
            get_redfish_sub_routes(app, "/redfish/v1/AccountService",
                                   res.json_value);
            res.end();
          });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Roles/Administrator/")
      .methods("GET"_method)(
          [&](const crow::request& req, crow::response& res) {
            res.json_value = {
                {"@odata.context", "/redfish/v1/$metadata#Role.Role"},
                {"@odata.id", "/redfish/v1/AccountService/Roles/Administrator"},
                {"@odata.type", "#Role.v1_0_2.Role"},
                {"Id", "Administrator"},
                {"Name", "User Role"},
                {"Description", "Administrator User Role"},
                {"IsPredefined", true},
                {"AssignedPrivileges",
                 {"Login", "ConfigureManager", "ConfigureUsers",
                  "ConfigureSelf", "ConfigureComponents"}}};
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
                for (auto& user : users) {
                  member_array.push_back(
                      {{"@odata.id",
                        "/redfish/v1/AccountService/Accounts/" +
                            std::to_string(++user_index)}});
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

  CROW_ROUTE(app, "/redfish/v1/SessionService/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        res.json_value = {
            {"@odata.context",
             "/redfish/v1/$metadata#SessionService.SessionService"},
            {"@odata.id", "/redfish/v1/SessionService"},
            {"@odata.type", "#SessionService.v1_1_1.SessionService"},
            {"Id", "SessionService"},
            {"Name", "SessionService"},
            {"Description", "SessionService"},
            {"Status",
             {{"State", "Enabled"}, {"Health", "OK"}, {"HealthRollup", "OK"}}},
            {"ServiceEnabled", true},
            // TODO(ed) converge with session timeouts once they exist
            // Bogus number for now
            {"SessionTimeout", 1800}};
        get_redfish_sub_routes(app, "/redfish/v1/AccountService",
                               res.json_value);
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/SessionService/Sessions/")
      .methods("POST"_method, "GET"_method)([&](const crow::request& req,
                                                crow::response& res) {
        auto& data_middleware =
            app.template get_middleware<PersistentData::Middleware>();
        if (req.method == "POST"_method) {
          // call with exceptions disabled
          auto login_credentials =
              nlohmann::json::parse(req.body, nullptr, false);
          if (login_credentials.is_discarded()) {
            res.code = 400;
            res.end();
            return;
          }
          // check for username/password in the root object
          auto user_it = login_credentials.find("UserName");
          auto pass_it = login_credentials.find("Password");
          if (user_it == login_credentials.end() ||
              pass_it == login_credentials.end()) {
            res.code = 400;
            res.end();
            return;
          }

          std::string username = user_it->get<const std::string>();
          std::string password = pass_it->get<const std::string>();
          if (username.empty() || password.empty()) {
            res.code = 400;
            res.end();
            return;
          }

          if (!pam_authenticate_user(username, password)) {
            res.code = 401;
            res.end();
            return;
          }
          auto session = data_middleware.generate_user_session(username);
          res.code = 200;
          res.add_header("X-Auth-Token", session.session_token);
          res.json_value = {
              {"@odata.context", "/redfish/v1/$metadata#Session"},
              {"@odata.id",
               "/redfish/v1/SessionService/Sessions/" + session.unique_id},
              {"@odata.type", "#Session.v1_0_3.Session"},
              {"Id", session.unique_id},
              {"Name", "User Session"},
              {"Description", "Manager User Session"},
              {"UserName", username}};
        } else {  // assume get
          res.json_value = {
              {"@odata.context",
               "/redfish/v1/$metadata#SessionCollection.SessionCollection"},
              {"@odata.id", "/redfish/v1/SessionService/Sessions"},
              {"@odata.type", "#SessionCollection.SessionCollection"},
              {"Name", "Session Collection"},
              {"Description", "Session Collection"},
              {"Members@odata.count", data_middleware.auth_tokens.size()}

          };
          nlohmann::json member_array = nlohmann::json::array();
          for (auto& session : data_middleware.auth_tokens) {
            member_array.push_back({{"@odata.id",
                                     "/redfish/v1/SessionService/Sessions/" +
                                         session.second.unique_id}});
          }
          res.json_value["Members"] = member_array;
        }
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/SessionService/Sessions/<str>/")
      .methods("GET"_method, "DELETE"_method)([&](const crow::request& req,
                                                  crow::response& res,
                                                  const std::string& session) {
        auto& data_middleware =
            app.template get_middleware<PersistentData::Middleware>();
        // TODO(Ed) this is inefficient
        auto session_it = data_middleware.auth_tokens.begin();
        while (session_it != data_middleware.auth_tokens.end()) {
          if (session_it->second.unique_id == session) {
            break;
          }
          session_it++;
        }

        if (session_it == data_middleware.auth_tokens.end()) {
          res.code = 404;
          res.end();
          return;
        }
        if (req.method == "DELETE"_method) {
          data_middleware.auth_tokens.erase(session_it);
          res.code = 200;
        } else {  // assume get
          res.json_value = {
              {"@odata.context", "/redfish/v1/$metadata#Session.Session"},
              {"@odata.id", "/redfish/v1/SessionService/Sessions/" + session},
              {"@odata.type", "#Session.v1_0_3.Session"},
              {"Id", session_it->second.unique_id},
              {"Name", "User Session"},
              {"Description", "Manager User Session"},
              {"UserName", session_it->second.username}};
        }
        res.end();
      });
}
}  // namespace redfish
}  // namespace crow
