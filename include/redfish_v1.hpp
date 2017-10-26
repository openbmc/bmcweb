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
                for (int user_index = 0; user_index < users.size();
                     user_index++) {
                  member_array.push_back(
                      {{"@odata.id",
                        "/redfish/v1/AccountService/Accounts/" +
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
        auto& session_store =
            app.template get_middleware<PersistentData::Middleware>().sessions;
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
          auto session = session_store.generate_user_session(username);
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
          std::vector<const std::string*> session_ids =
              session_store.get_unique_ids();
          res.json_value = {
              {"@odata.context",
               "/redfish/v1/$metadata#SessionCollection.SessionCollection"},
              {"@odata.id", "/redfish/v1/SessionService/Sessions"},
              {"@odata.type", "#SessionCollection.SessionCollection"},
              {"Name", "Session Collection"},
              {"Description", "Session Collection"},
              {"Members@odata.count", session_ids.size()}

          };
          nlohmann::json member_array = nlohmann::json::array();
          for (auto session_uid : session_ids) {
            member_array.push_back(
                {{"@odata.id",
                  "/redfish/v1/SessionService/Sessions/" + *session_uid}});
          }
          res.json_value["Members"] = member_array;
        }
        res.end();
      });

  CROW_ROUTE(app, "/redfish/v1/SessionService/Sessions/<str>/")
      .methods("GET"_method, "DELETE"_method)([&](
          const crow::request& req, crow::response& res,
          const std::string& session_id) {
        auto& session_store =
            app.template get_middleware<PersistentData::Middleware>().sessions;
        // TODO(Ed) this is inefficient
        auto session = session_store.get_session_by_uid(session_id);

        if (session == nullptr) {
          res.code = 404;
          res.end();
          return;
        }
        if (req.method == "DELETE"_method) {
          session_store.remove_session(session);
          res.code = 200;
        } else {  // assume get
          res.json_value = {
              {"@odata.context", "/redfish/v1/$metadata#Session.Session"},
              {"@odata.id",
               "/redfish/v1/SessionService/Sessions/" + session->unique_id},
              {"@odata.type", "#Session.v1_0_3.Session"},
              {"Id", session->unique_id},
              {"Name", "User Session"},
              {"Description", "Manager User Session"},
              {"UserName", session->username}};
        }
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
            {"UUID",
             app.template get_middleware<PersistentData::Middleware>()
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

  CROW_ROUTE(app, "/redfish/v1/Managers/NetworkProtocol/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        std::array<char, HOST_NAME_MAX> hostname;
        if (gethostname(hostname.data(), hostname.size()) != 0) {
          res.code = 500;
          res.end();
          return;
        }
        res.json_value = {
            {"@odata.context",
             "/redfish/v1/"
             "$metadata#ManagerNetworkProtocol.ManagerNetworkProtocol"},
            {"@odata.id", "/redfish/v1/Managers/BMC/NetworkProtocol"},
            {"@odata.type",
             "#ManagerNetworkProtocol.v1_1_0.ManagerNetworkProtocol"},
            {"Id", "NetworkProtocol"},
            {"Name", "Manager Network Protocol"},
            {"Description", "Manager Network Service"},
            {"Status",
             {{"State", "Enabled"}, {"Health", "OK"}, {"HealthRollup", "OK"}}},
            {"HostName", hostname.data()}};  // TODO(ed) get hostname
        std::string netstat_out = execute_process("netstat -tuln");

        std::map<int, const char*> service_types{{22, "SSH"},
                                                 {443, "HTTPS"},
                                                 {1900, "SSDP"},
                                                 {623, "IPMI"},
                                                 {427, "SLP"}};

        std::vector<std::string> lines;
        boost::split(lines, netstat_out, boost::is_any_of("\n"));
        auto lines_it = lines.begin();
        lines_it++;  // skip the netstat header
        lines_it++;
        while (lines_it != lines.end()) {
          std::vector<std::string> columns;
          boost::split(columns, *lines_it, boost::is_any_of("\t "),
                       boost::token_compress_on);
          if (columns.size() >= 5) {
            std::size_t found = columns[3].find_last_of(":");
            if (found != std::string::npos) {
              std::string port_str = columns[3].substr(found + 1);
              int port = std::stoi(port_str.c_str());
              auto type_it = service_types.find(port);
              if (type_it != service_types.end()) {
                res.json_value[type_it->second] = {{"ProtocolEnabled", true},
                                                   {"Port", port}};
              }
            }
          }
          lines_it++;
        }

        get_redfish_sub_routes(app, "/redfish/v1/", res.json_value);
        res.end();
      });
}
}  // namespace redfish
}  // namespace crow
