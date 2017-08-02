#include <crow/app.h>
namespace crow {
namespace redfish {

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
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
         {{"State", "Enabled"}, {"Health", "OK"}, {"HealthRollup", "OK"}}},
        {"ServiceEnabled", true},
        {"MinPasswordLength", 1},
        {"MaxPasswordLength", 20},
        {"Accounts", {{"@odata.id", "/redfish/v1/AccountService/Accounts"}}},
        //{"Roles", {{"@odata.id", "/redfish/v1/AccountService/Roles"}}}
    };
  });

  CROW_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
      .methods("GET"_method)([]() {
        return nlohmann::json{
            {"@odata.context",
             "/redfish/v1/"
             "$metadata#ManagerAccountCollection.ManagerAccountCollection"},
            {"@odata.id", "/redfish/v1/AccountService/Accounts"},
            {"@odata.type",
             "#ManagerAccountCollection.ManagerAccountCollection"},
            {"Name", "Accounts Collection"},
            {"Description", "BMC User Accounts"},
            {"Members@odata.count", 3},
            {"Members",
             {{{"@odata.id", "/redfish/v1/AccountService/Accounts/1"}},
              {{"@odata.id", "/redfish/v1/AccountService/Accounts/2"}},
              {{"@odata.id", "/redfish/v1/AccountService/Accounts/3"}}}}};
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
}
}