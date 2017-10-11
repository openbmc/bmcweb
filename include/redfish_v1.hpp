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

  CROW_ROUTE(app, "/redfish/v1/$metadata/")
      .methods(
          "GET"_method)([&](const crow::request& req, crow::response& res) {
        const char* response_text = R"(<?xml version="1.0" encoding="UTF-8"?>
        <edmx:Edmx xmlns:edmx="http://docs.oasis-open.org/odata/ns/edmx" Version="4.0">
            <edmx:Reference Uri="/redfish/v1/schema/ServiceRoot_v1.xml">
                <edmx:Include Namespace="ServiceRoot"/>
                <edmx:Include Namespace="ServiceRoot.v1_0_4"/>
                <edmx:Include Namespace="ServiceRoot.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/AccountService_v1.xml">
                <edmx:Include Namespace="AccountService"/>
                <edmx:Include Namespace="AccountService.v1_0_3"/>
                <edmx:Include Namespace="AccountService.v1_1_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Chassis_v1.xml">
                <edmx:Include Namespace="Chassis"/>
                <edmx:Include Namespace="Chassis.v1_0_3"/>
                <edmx:Include Namespace="Chassis.v1_1_3"/>
                <edmx:Include Namespace="Chassis.v1_2_1"/>
                <edmx:Include Namespace="Chassis.v1_3_1"/>
                <edmx:Include Namespace="Chassis.v1_4_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ChassisCollection_v1.xml">
                <edmx:Include Namespace="ChassisCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ComputerSystem_v1.xml">
                <edmx:Include Namespace="ComputerSystem"/>
                <edmx:Include Namespace="ComputerSystem.v1_0_4"/>
                <edmx:Include Namespace="ComputerSystem.v1_1_2"/>
                <edmx:Include Namespace="ComputerSystem.v1_2_1"/>
                <edmx:Include Namespace="ComputerSystem.v1_3_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ComputerSystemCollection_v1.xml">
                <edmx:Include Namespace="ComputerSystemCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/EthernetInterface_v1.xml">
                <edmx:Include Namespace="EthernetInterface"/>
                <edmx:Include Namespace="EthernetInterface.v1_0_3"/>
                <edmx:Include Namespace="EthernetInterface.v1_1_1"/>
                <edmx:Include Namespace="EthernetInterface.v1_2_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/EthernetInterfaceCollection_v1.xml">
                <edmx:Include Namespace="EthernetInterfaceCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/LogEntry_v1.xml">
                <edmx:Include Namespace="LogEntry"/>
                <edmx:Include Namespace="LogEntry.v1_0_3"/>
                <edmx:Include Namespace="LogEntry.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/LogEntryCollection_v1.xml">
                <edmx:Include Namespace="LogEntryCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/LogService_v1.xml">
                <edmx:Include Namespace="LogService"/>
                <edmx:Include Namespace="LogService.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/LogServiceCollection_v1.xml">
                <edmx:Include Namespace="LogServiceCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Manager_v1.xml">
                <edmx:Include Namespace="Manager"/>
                <edmx:Include Namespace="Manager.v1_0_3"/>
                <edmx:Include Namespace="Manager.v1_1_1"/>
                <edmx:Include Namespace="Manager.v1_2_1"/>
                <edmx:Include Namespace="Manager.v1_3_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ManagerAccount_v1.xml">
                <edmx:Include Namespace="ManagerAccount"/>
                <edmx:Include Namespace="ManagerAccount.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ManagerNetworkProtocol_v1.xml">
                <edmx:Include Namespace="ManagerNetworkProtocol"/>
                <edmx:Include Namespace="ManagerNetworkProtocol.v1_0_3"/>
                <edmx:Include Namespace="ManagerNetworkProtocol.v1_1_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ManagerAccountCollection_v1.xml">
                <edmx:Include Namespace="ManagerAccountCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ManagerCollection_v1.xml">
                <edmx:Include Namespace="ManagerCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Power_v1.xml">
                <edmx:Include Namespace="Power"/>
                <edmx:Include Namespace="Power.v1_0_3"/>
                <edmx:Include Namespace="Power.v1_1_1"/>
                <edmx:Include Namespace="Power.v1_2_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Processor_v1.xml">
                <edmx:Include Namespace="Processor"/>
                <edmx:Include Namespace="Processor.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/ProcessorCollection_v1.xml">
                <edmx:Include Namespace="ProcessorCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Role_v1.xml">
                <edmx:Include Namespace="Role"/>
                <edmx:Include Namespace="Role.v1_0_2"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/RoleCollection_v1.xml">
                <edmx:Include Namespace="RoleCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Session_v1.xml">
                <edmx:Include Namespace="Session"/>
                <edmx:Include Namespace="Session.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/SessionCollection_v1.xml">
                <edmx:Include Namespace="SessionCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/SessionService_v1.xml">
                <edmx:Include Namespace="SessionService"/>
                <edmx:Include Namespace="SessionService.v1_0_3"/>
                <edmx:Include Namespace="SessionService.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Thermal_v1.xml">
                <edmx:Include Namespace="Thermal"/>
                <edmx:Include Namespace="Thermal.v1_0_3"/>
                <edmx:Include Namespace="Thermal.v1_1_1"/>
                <edmx:Include Namespace="Thermal.v1_2_0"/>
            </edmx:Reference>
          <edmx:Reference Uri="/redfish/v1/schema/RedfishExtensions_v1.xml">
                <edmx:Include Namespace="RedfishExtensions.v1_0_0" Alias="Redfish"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/IPAddresses_v1.xml">
                <edmx:Include Namespace="IPAddresses"/>
                <edmx:Include Namespace="IPAddresses.v1_0_4"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/MemoryCollection_v1.xml">
                <edmx:Include Namespace="MemoryCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/MemoryMetrics_v1.xml">
                <edmx:Include Namespace="MemoryMetrics"/>
                <edmx:Include Namespace="MemoryMetrics.v1_0_1"/>
                <edmx:Include Namespace="MemoryMetrics.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Memory_v1.xml">
                <edmx:Include Namespace="Memory"/>
                <edmx:Include Namespace="Memory.v1_0_1"/>
                <edmx:Include Namespace="Memory.v1_1_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/PhysicalContext_v1.xml">
                <edmx:Include Namespace="PhysicalContext"/>
                <edmx:Include Namespace="PhysicalContext.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Privileges_v1.xml">
                <edmx:Include Namespace="Privileges"/>
                <edmx:Include Namespace="Privileges.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Redundancy_v1.xml">
                <edmx:Include Namespace="Redundancy"/>
                <edmx:Include Namespace="Redundancy.v1_0_3"/>
                <edmx:Include Namespace="Redundancy.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Resource_v1.xml">
                <edmx:Include Namespace="Resource"/>
                <edmx:Include Namespace="Resource.v1_0_3"/>
                <edmx:Include Namespace="Resource.v1_1_2"/>
                <edmx:Include Namespace="Resource.v1_2_1"/>
                <edmx:Include Namespace="Resource.v1_3_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/VirtualMediaCollection_v1.xml">
                <edmx:Include Namespace="VirtualMediaCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/VirtualMedia_v1.xml">
                <edmx:Include Namespace="VirtualMedia"/>
                <edmx:Include Namespace="VirtualMedia.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/VLanNetworkInterfaceCollection_v1.xml">
                <edmx:Include Namespace="VLanNetworkInterfaceCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/VLanNetworkInterface_v1.xml">
                <edmx:Include Namespace="VLanNetworkInterface"/>
                <edmx:Include Namespace="VLanNetworkInterface.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/StorageCollection_v1.xml">
                <edmx:Include Namespace="StorageCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Storage_v1.xml">
                <edmx:Include Namespace="Storage"/>
                <edmx:Include Namespace="Storage.v1_0_2"/>
                <edmx:Include Namespace="Storage.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Drive_v1.xml">
                <edmx:Include Namespace="Drive"/>
                <edmx:Include Namespace="Drive.v1_0_2"/>
                <edmx:Include Namespace="Drive.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/SoftwareInventoryCollection_v1.xml">
                <edmx:Include Namespace="SoftwareInventoryCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/SoftwareInventory_v1.xml">
                <edmx:Include Namespace="SoftwareInventory"/>
                <edmx:Include Namespace="SoftwareInventory.v1_0_1"/>
                <edmx:Include Namespace="SoftwareInventory.v1_1_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/UpdateService_v1.xml">
                <edmx:Include Namespace="UpdateService"/>
                <edmx:Include Namespace="UpdateService.v1_0_1"/>
                <edmx:Include Namespace="UpdateService.v1_1_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Message_v1.xml">
                <edmx:Include Namespace="Message"/>
                <edmx:Include Namespace="Message.v1_0_4"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/EndpointCollection_v1.xml">
                <edmx:Include Namespace="EndpointCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/Endpoint_v1.xml">
                <edmx:Include Namespace="Endpoint"/>
                <edmx:Include Namespace="Endpoint.v1_0_1"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/HostInterfaceCollection_v1.xml">
                <edmx:Include Namespace="HostInterfaceCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/HostInterface_v1.xml">
                <edmx:Include Namespace="HostInterface"/>
                <edmx:Include Namespace="HostInterface.v1_0_0"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/MessageRegistryFileCollection_v1.xml">
                <edmx:Include Namespace="MessageRegistryFileCollection"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/MessageRegistryFile_v1.xml">
                <edmx:Include Namespace="MessageRegistryFile"/>
                <edmx:Include Namespace="MessageRegistryFile.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/EventService_v1.xml">
                <edmx:Include Namespace="EventService"/>
                <edmx:Include Namespace="EventService.v1_0_3"/>
            </edmx:Reference>
            <edmx:Reference Uri="/redfish/v1/schema/EventDestinationCollection_v1.xml">
                <edmx:Include Namespace="EventDestinationCollection"/>
            </edmx:Reference> 
            <edmx:Reference Uri="/redfish/v1/schema/EventDestination_v1.xml">
                <edmx:Include Namespace="EventDestination"/>
                <edmx:Include Namespace="EventDestination.v1_0_3"/>
                <edmx:Include Namespace="EventDestination.v1_1_1"/>
            </edmx:Reference> 
            <edmx:Reference Uri="/redfish/v1/schema/Event_v1.xml">
                <edmx:Include Namespace="Event"/>
                <edmx:Include Namespace="Event.v1_0_4"/>
                <edmx:Include Namespace="Event.v1_1_2"/>
            </edmx:Reference> 
            <edmx:DataServices>
                <Schema xmlns="http://docs.oasis-open.org/odata/ns/edm" Namespace="Service">
                    <EntityContainer Name="Service" Extends="ServiceRoot.v1_0_0.ServiceContainer"/>
                </Schema>
            </edmx:DataServices>
        </edmx:Edmx>)";

        res.body = response_text;
        res.add_header("Content-Type", "application/xml");
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
        auto session_it = data_middleware.auth_tokens.find(session);
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
