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

#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <openbmc_dbus_rest.hpp>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

constexpr const char* ldapConfigObject =
    "/xyz/openbmc_project/user/ldap/config";
constexpr const char* ldapRootObject = "/xyz/openbmc_project/user/ldap";
constexpr const char* ldapDbusService = "xyz.openbmc_project.Ldap.Config";
constexpr const char* ldapConfigInterface =
    "xyz.openbmc_project.User.Ldap.Config";
constexpr const char* ldapCreateInterface =
    "xyz.openbmc_project.User.Ldap.Create";
constexpr const char* ldapEnableInterface = "xyz.openbmc_project.Object.Enable";
constexpr const char* dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
constexpr const char* propertyInterface = "org.freedesktop.DBus.Properties";
constexpr const char* mapperBusName = "xyz.openbmc_project.ObjectMapper";
constexpr const char* mapperObjectPath = "/xyz/openbmc_project/object_mapper";
constexpr const char* mapperIntf = "xyz.openbmc_project.ObjectMapper";

struct LDAPConfigData
{
    std::string uri{};
    std::string bindDN{};
    std::string baseDN{};
    std::string searchScope{};
    std::string serverType{};
    bool serviceEnabled = false;
    std::string userNameAttribute{};
    std::string groupAttribute{};
};

using ManagedObjectType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<
                         std::string, std::variant<bool, std::string>>>>>;
using GetObjectType =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

template <typename Callback>
inline void checkDbusPathExists(const std::string& path, Callback&& callback)
{
    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code ec,
                                        const GetObjectType& object_names) {
            callback(!ec && object_names.size() != 0);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

inline std::string getPrivilegeFromRoleId(boost::beast::string_view role)
{
    if (role == "priv-admin")
    {
        return "Administrator";
    }
    else if (role == "priv-callback")
    {
        return "Callback";
    }
    else if (role == "priv-user")
    {
        return "User";
    }
    else if (role == "priv-operator")
    {
        return "Operator";
    }
    return "";
}
inline std::string getRoleIdFromPrivilege(boost::beast::string_view role)
{
    if (role == "Administrator")
    {
        return "priv-admin";
    }
    else if (role == "Callback")
    {
        return "priv-callback";
    }
    else if (role == "User")
    {
        return "priv-user";
    }
    else if (role == "Operator")
    {
        return "priv-operator";
    }
    return "";
}

void parseLDAPConfigData(nlohmann::json& json_response,
                         const LDAPConfigData& confData)
{
    std::string service = "LDAPService";
    json_response["LDAP"] = {
        {"AccountProviderType", service},
        {"AccountProviderType@Redfish.AllowableValues",
         nlohmann::json::array({service})},
        {"ServiceEnabled", confData.serviceEnabled},
        {"ServiceAddresses", nlohmann::json::array({confData.uri})},
        {"Authentication",
         {{"AuthenticationType", "UsernameAndPassword"},
          {"AuthenticationType@Redfish.AllowableValues",
           nlohmann::json::array({"UsernameAndPassword"})},
          {"Username", confData.bindDN},
          {"Password", nullptr}}},
        {"LDAPService",
         {{"SearchSettings",
           {{"BaseDistinguishedNames",
             nlohmann::json::array({confData.baseDN})},
            {"UsernameAttribute", confData.userNameAttribute},
            {"GroupsAttribute", confData.groupAttribute}}}}},
    };
}

/**
 * Function that retrieves all properties for LDAP config object
 * into JSON
 */
template <typename CallbackFunc>
inline void getLDAPConfigData(CallbackFunc&& callback)
{
    auto getConfig = [callback](const boost::system::error_code error_code,
                                const ManagedObjectType& ldapObjects) {
        LDAPConfigData confData{};
        if (error_code)
        {
            callback(false, confData);
            BMCWEB_LOG_ERROR << "D-Bus responses error: " << error_code;
            return;
        }
        std::string ldapConfigObjectStr = std::string(ldapConfigObject);
        std::string ldapEnableInterfaceStr = std::string(ldapEnableInterface);
        std::string ldapConfigInterfaceStr = std::string(ldapConfigInterface);
        for (const auto& object : ldapObjects)
        {
            if (object.first == ldapConfigObjectStr)
            {
                for (const auto& interface : object.second)
                {
                    if (interface.first == ldapEnableInterfaceStr)
                    {
                        // rest of the properties are string.
                        for (const auto& property : interface.second)
                        {
                            if (property.first == "Enabled")
                            {
                                const bool* value =
                                    std::get_if<bool>(&property.second);
                                if (value == nullptr)
                                {
                                    continue;
                                }
                                confData.serviceEnabled = *value;
                                break;
                            }
                        }
                    }
                    else if (interface.first == ldapConfigInterfaceStr)
                    {

                        for (const auto& property : interface.second)
                        {
                            const std::string* value =
                                std::get_if<std::string>(&property.second);
                            if (value == nullptr)
                            {
                                continue;
                            }
                            if (property.first == "LDAPServerURI")
                            {
                                confData.uri = *value;
                            }
                            else if (property.first == "LDAPBindDN")
                            {
                                confData.bindDN = *value;
                            }
                            else if (property.first == "LDAPBaseDN")
                            {
                                confData.baseDN = *value;
                            }
                            else if (property.first == "LDAPSearchScope")
                            {
                                confData.searchScope = *value;
                            }
                            else if (property.first == "LDAPType")
                            {
                                confData.serverType = *value;
                            }
                            else if (property.first == "GroupNameAttribute")
                            {
                                confData.groupAttribute = *value;
                            }
                            else if (property.first == "UserNameAttribute")
                            {
                                confData.userNameAttribute = *value;
                            }
                        }
                    }
                }

                callback(true, confData);
                break;
            }
        }
    };
    auto getServiceName = [callback, getConfig(std::move(getConfig))](
                              const boost::system::error_code ec,
                              const GetObjectType& resp) {
        LDAPConfigData confData{};
        if (ec || resp.empty())
        {
            BMCWEB_LOG_ERROR
                << "DBUS response error during getting of service name: " << ec;
            callback(false, confData);
            return;
        }
        std::string service = resp.begin()->first;
        crow::connections::systemBus->async_method_call(
            std::move(getConfig), service, ldapRootObject, dbusObjManagerIntf,
            "GetManagedObjects");
    };

    const std::array<std::string, 2> interfaces = {ldapEnableInterface,
                                                   ldapConfigInterface};

    crow::connections::systemBus->async_method_call(
        std::move(getServiceName), mapperBusName, mapperObjectPath, mapperIntf,
        "GetObject", ldapConfigObject, interfaces);
}

class AccountService : public Node
{
  public:
    AccountService(CrowApp& app) : Node(app, "/redfish/v1/AccountService/")
    {
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
    /**
     * @brief Get the required values from the given JSON, validates the
     *        value and create the LDAP config object.
     * @param input JSON data
     * @param asyncResp pointer to the JSON response
     * @param serverType Type of LDAP server(openLDAP/ActiveDirectory)
     */

    void handleLDAPPatch(const nlohmann::json& input,
                         const std::shared_ptr<AsyncResp>& asyncResp,
                         const crow::Request& req,
                         const std::vector<std::string>& params,
                         const std::string& serverType)
    {
        // NOTE: Currently we are expecting the user to provide all the data
        // under LDAP property as we have backend limitation which starts the
        // certain processes after each property update.

        std::string dbusObjectPath = "/xyz/openbmc_project/user/ldap/config";
        dbus::utility::escapePathForDbus(dbusObjectPath);

        checkDbusPathExists(dbusObjectPath, [this, input, asyncResp, req,
                                             params, serverType](int rc) {
            if (rc) // if path exist
            {
                // TODO(ratagupt) handling of individual property would be done
                // in later commit.
                BMCWEB_LOG_DEBUG << "D-Bus object exist still forces user to "
                                    "give all the LDAP config data.";
            }
            else
            {
                // D-bus object path doesn't exist so we need all the
                // required properties from the user.
                BMCWEB_LOG_DEBUG << "D-Bus object doesn't exist";
            }

            std::optional<nlohmann::json> authentication;
            std::optional<nlohmann::json> ldapService;
            std::optional<std::string> accountProviderType;
            std::optional<std::vector<std::string>> serviceAddressList;
            // by default service is disabled;
            std::optional<bool> serviceEnabled = false;

            if (!json_util::readJson(const_cast<nlohmann::json&>(input),
                                     asyncResp->res, "Authentication",
                                     authentication, "LDAPService", ldapService,
                                     "ServiceAddresses", serviceAddressList,
                                     "AccountProviderType", accountProviderType,
                                     "ServiceEnabled", serviceEnabled))
            {
                return;
            }

            if (!authentication)
            {
                messages::propertyMissing(asyncResp->res, "Authentication");
                return;
            }

            if (!ldapService)
            {
                messages::propertyMissing(asyncResp->res, "LDAPService");
                return;
            }

            if (!serviceAddressList)
            {
                messages::propertyMissing(asyncResp->res, "ServiceAddresses");
                return;
            }

            if (accountProviderType)
            {
                messages::propertyNotWritable(asyncResp->res,
                                              "AccountProviderType");
            }

            std::string serviceAddress;
            bool isServiceModified = false;
            if (!(*serviceAddressList).empty())
            {

                // Currently backend supports the single LDAP server
                // suppose if user has given more then one LDAP server address
                // we will use the one at front and give the user a message
                // saying that service address is modified.

                if ((*serviceAddressList).size() > 1)
                {
                    isServiceModified = true;
                }
                serviceAddress = (*serviceAddressList).front();
            }

            std::optional<std::string> username;
            std::optional<std::string> password;
            std::optional<std::string> authType;

            if (!json_util::readJson(*authentication, asyncResp->res,
                                     "AuthenticationType", authType, "Username",
                                     username, "Password", password))
            {
                return;
            }

            if (!authType)
            {
                messages::propertyMissing(asyncResp->res, "AuthenticationType");
                return;
            }

            if (!username)
            {
                messages::propertyMissing(asyncResp->res, "Username");
                return;
            }

            if (!password)
            {
                messages::propertyMissing(asyncResp->res, "Password");
                return;
            }

            if (*authType != "UsernameAndPassword")
            {
                messages::propertyValueNotInList(asyncResp->res, *authType,
                                                 "AuthenticationType");
                return;
            }

            std::optional<nlohmann::json> searchSettings;

            if (!json_util::readJson(*ldapService, asyncResp->res,
                                     "SearchSettings", searchSettings))
            {
                return;
            }
            if (!searchSettings)
            {
                messages::propertyMissing(asyncResp->res, "SearchSettings");
                return;
            }

            std::optional<std::vector<std::string>> baseDNList;
            std::optional<std::string> userNameAttribute;
            std::optional<std::string> groupsAttribute;

            if (!json_util::readJson(*searchSettings, asyncResp->res,
                                     "BaseDistinguishedNames", baseDNList,
                                     "UsernameAttribute", userNameAttribute,
                                     "GroupsAttribute", groupsAttribute))
            {
                return;
            }
            if (!baseDNList)
            {
                messages::propertyMissing(asyncResp->res,
                                          "BaseDistinguishedNames");
                return;
            }
            // assign the empty string for the username and groups attribute
            // if not given by the user.
            if (!userNameAttribute)
            {
                userNameAttribute = "";
            }
            if (!groupsAttribute)
            {
                groupsAttribute = "";
            }

            std::string baseDN;
            bool isBaseDNModified = false;
            if (!(*baseDNList).empty())
            {
                // Currently backend supports the single baseDN
                // suppose if user has given more then one baseDN
                // we will use the one at front and give the user a message
                // saying that baseDN is modified.
                if ((*baseDNList).size() > 1)
                {
                    isBaseDNModified = true;
                }
                baseDN = (*baseDNList).front();
            }

            std::string server =
                "xyz.openbmc_project.User.Ldap.Create.Type.OpenLdap";

            auto enableService = [this, asyncResp, serviceEnabled,
                                  serverType]() {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, serviceEnabled,
                     serverType](const boost::system::error_code ec) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        else
                        {
                            BMCWEB_LOG_DEBUG << "ServiceEnabled="
                                             << *serviceEnabled;
                            asyncResp->res
                                .jsonValue[serverType]["ServiceEnabled"] =
                                *serviceEnabled;
                        }
                    },
                    ldapDbusService, ldapConfigObject, propertyInterface, "Set",
                    ldapEnableInterface, "Enabled",
                    std::variant<bool>(*serviceEnabled));
            };

            auto createLDAPConfigHandler =
                [asyncResp, serviceAddress, baseDN, isServiceModified,
                 isBaseDNModified, enableService = std::move(enableService)](
                    const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    // Get the modified resource and send it back.
                    getLDAPConfigData(
                        [baseDN = std::move(baseDN),
                         serviceAddress = std::move(serviceAddress),
                         isServiceModified, isBaseDNModified, asyncResp,
                         enableService = std::move(enableService)](
                            bool success, LDAPConfigData confData) {
                            if (!success)
                            {
                                BMCWEB_LOG_DEBUG << "Ldap Config doesn't exist";
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            parseLDAPConfigData(asyncResp->res.jsonValue,
                                                confData);
                            if (isServiceModified)
                            {
                                messages::propertyValueModified(
                                    asyncResp->res, "ServiceAddresses",
                                    serviceAddress);
                            }
                            if (isBaseDNModified)
                            {
                                messages::propertyValueModified(
                                    asyncResp->res, "BaseDistinguishedNames",
                                    baseDN);
                            }
                            enableService();
                            BMCWEB_LOG_DEBUG << "Created the LDAP Config";
                        });
                };

            crow::connections::systemBus->async_method_call(
                std::move(createLDAPConfigHandler), ldapDbusService,
                ldapRootObject, ldapCreateInterface, "CreateConfig",
                serviceAddress, *username, baseDN, *password,
                "xyz.openbmc_project.User.Ldap.Create.SearchScope.sub", server,
                *userNameAttribute, *groupsAttribute);
        });
    }

    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue = {
            {"@odata.context", "/redfish/v1/"
                               "$metadata#AccountService.AccountService"},
            {"@odata.id", "/redfish/v1/AccountService"},
            {"@odata.type", "#AccountService."
                            "v1_3_1.AccountService"},
            {"Id", "AccountService"},
            {"Name", "Account Service"},
            {"Description", "Account Service"},
            {"ServiceEnabled", true},
            {"MaxPasswordLength", 20},
            {"Accounts",
             {{"@odata.id", "/redfish/v1/AccountService/Accounts"}}},
            {"Roles", {{"@odata.id", "/redfish/v1/AccountService/Roles"}}}};

        crow::connections::systemBus->async_method_call(
            [asyncResp](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::variant<uint32_t, uint16_t, uint8_t>>>&
                    propertiesList) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                                 << "properties for AccountService";
                for (const std::pair<std::string,
                                     std::variant<uint32_t, uint16_t, uint8_t>>&
                         property : propertiesList)
                {
                    if (property.first == "MinPasswordLength")
                    {
                        const uint8_t* value =
                            std::get_if<uint8_t>(&property.second);
                        if (value != nullptr)
                        {
                            asyncResp->res.jsonValue["MinPasswordLength"] =
                                *value;
                        }
                    }
                    if (property.first == "AccountUnlockTimeout")
                    {
                        const uint32_t* value =
                            std::get_if<uint32_t>(&property.second);
                        if (value != nullptr)
                        {
                            asyncResp->res.jsonValue["AccountLockoutDuration"] =
                                *value;
                        }
                    }
                    if (property.first == "MaxLoginAttemptBeforeLockout")
                    {
                        const uint16_t* value =
                            std::get_if<uint16_t>(&property.second);
                        if (value != nullptr)
                        {
                            asyncResp->res
                                .jsonValue["AccountLockoutThreshold"] = *value;
                        }
                    }
                }
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "org.freedesktop.DBus.Properties", "GetAll",
            "xyz.openbmc_project.User.AccountPolicy");

        getLDAPConfigData([asyncResp](bool success, LDAPConfigData& confData) {
            parseLDAPConfigData(asyncResp->res.jsonValue, confData);
        });
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        std::optional<uint32_t> unlockTimeout;
        std::optional<uint16_t> lockoutThreshold;
        std::optional<uint16_t> minPasswordLength;
        std::optional<uint16_t> maxPasswordLength;
        std::optional<nlohmann::json> ldapObject;

        if (!json_util::readJson(req, res, "AccountLockoutDuration",
                                 unlockTimeout, "AccountLockoutThreshold",
                                 lockoutThreshold, "MaxPasswordLength",
                                 maxPasswordLength, "MinPasswordLength",
                                 minPasswordLength, "LDAP", ldapObject))
        {
            return;
        }

        if (minPasswordLength)
        {
            messages::propertyNotWritable(asyncResp->res, "MinPasswordLength");
        }

        if (maxPasswordLength)
        {
            messages::propertyNotWritable(asyncResp->res, "MaxPasswordLength");
        }

        if (ldapObject)
        {
            handleLDAPPatch(*ldapObject, asyncResp, req, params, "LDAP");
        }

        if (unlockTimeout)
        {
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.User.AccountPolicy",
                "AccountUnlockTimeout", std::variant<uint32_t>(*unlockTimeout));
        }
        if (lockoutThreshold)
        {
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    messages::success(asyncResp->res);
                },
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.User.AccountPolicy",
                "MaxLoginAttemptBeforeLockout",
                std::variant<uint16_t>(*lockoutThreshold));
        }
    }
};
class AccountsCollection : public Node
{
  public:
    AccountsCollection(CrowApp& app) :
        Node(app, "/redfish/v1/AccountService/Accounts/")
    {
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
        auto asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue = {{"@odata.context",
                          "/redfish/v1/"
                          "$metadata#ManagerAccountCollection."
                          "ManagerAccountCollection"},
                         {"@odata.id", "/redfish/v1/AccountService/Accounts"},
                         {"@odata.type", "#ManagerAccountCollection."
                                         "ManagerAccountCollection"},
                         {"Name", "Accounts Collection"},
                         {"Description", "BMC User Accounts"}};

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const ManagedObjectType& users) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
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
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        std::string username;
        std::string password;
        std::optional<std::string> roleId("User");
        std::optional<bool> enabled = true;
        if (!json_util::readJson(req, res, "UserName", username, "Password",
                                 password, "RoleId", roleId, "Enabled",
                                 enabled))
        {
            return;
        }

        std::string priv = getRoleIdFromPrivilege(*roleId);
        if (priv.empty())
        {
            messages::propertyValueNotInList(asyncResp->res, *roleId, "RoleId");
            return;
        }
        roleId = priv;

        crow::connections::systemBus->async_method_call(
            [asyncResp, username, password{std::move(password)}](
                const boost::system::error_code ec) {
                if (ec)
                {
                    messages::resourceAlreadyExists(
                        asyncResp->res, "#ManagerAccount.v1_0_3.ManagerAccount",
                        "UserName", username);
                    return;
                }

                if (!pamUpdatePassword(username, password))
                {
                    // At this point we have a user that's been created, but the
                    // password set failed.  Something is wrong, so delete the
                    // user that we've already created
                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            messages::invalidObject(asyncResp->res, "Password");
                        },
                        "xyz.openbmc_project.User.Manager",
                        "/xyz/openbmc_project/user/" + username,
                        "xyz.openbmc_project.Object.Delete", "Delete");

                    BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                    return;
                }

                messages::created(asyncResp->res);
                asyncResp->res.addHeader(
                    "Location",
                    "/redfish/v1/AccountService/Accounts/" + username);
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.Manager", "CreateUser", username,
            std::array<const char*, 4>{"ipmi", "redfish", "ssh", "web"},
            *roleId, *enabled);
    }
};

class ManagerAccount : public Node
{
  public:
    ManagerAccount(CrowApp& app) :
        Node(app, "/redfish/v1/AccountService/Accounts/<str>/", std::string())
    {
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
        res.jsonValue = {
            {"@odata.context",
             "/redfish/v1/$metadata#ManagerAccount.ManagerAccount"},
            {"@odata.type", "#ManagerAccount.v1_0_3.ManagerAccount"},
            {"Name", "User Account"},
            {"Description", "User Account"},
            {"Password", nullptr},
            {"RoleId", "Administrator"}};

        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, accountName{std::string(params[0])}](
                const boost::system::error_code ec,
                const ManagedObjectType& users) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                auto userIt = users.begin();

                for (; userIt != users.end(); userIt++)
                {
                    if (boost::ends_with(userIt->first.str, "/" + accountName))
                    {
                        break;
                    }
                }
                if (userIt == users.end())
                {
                    messages::resourceNotFound(asyncResp->res, "ManagerAccount",
                                               accountName);
                    return;
                }
                for (const auto& interface : userIt->second)
                {
                    if (interface.first ==
                        "xyz.openbmc_project.User.Attributes")
                    {
                        for (const auto& property : interface.second)
                        {
                            if (property.first == "UserEnabled")
                            {
                                const bool* userEnabled =
                                    std::get_if<bool>(&property.second);
                                if (userEnabled == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "UserEnabled wasn't a bool";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                asyncResp->res.jsonValue["Enabled"] =
                                    *userEnabled;
                            }
                            else if (property.first ==
                                     "UserLockedForFailedAttempt")
                            {
                                const bool* userLocked =
                                    std::get_if<bool>(&property.second);
                                if (userLocked == nullptr)
                                {
                                    BMCWEB_LOG_ERROR << "UserLockedForF"
                                                        "ailedAttempt "
                                                        "wasn't a bool";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                asyncResp->res.jsonValue["Locked"] =
                                    *userLocked;
                                asyncResp->res.jsonValue
                                    ["Locked@Redfish.AllowableValues"] = {
                                    false};
                            }
                            else if (property.first == "UserPrivilege")
                            {
                                const std::string* userRolePtr =
                                    std::get_if<std::string>(&property.second);
                                if (userRolePtr == nullptr)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "UserPrivilege wasn't a "
                                           "string";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                std::string priv =
                                    getPrivilegeFromRoleId(*userRolePtr);
                                if (priv.empty())
                                {
                                    BMCWEB_LOG_ERROR << "Invalid user role";
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                asyncResp->res.jsonValue["RoleId"] = priv;

                                asyncResp->res.jsonValue["Links"]["Role"] = {
                                    {"@odata.id", "/redfish/v1/AccountService/"
                                                  "Roles/" +
                                                      priv}};
                            }
                        }
                    }
                }

                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/AccountService/Accounts/" + accountName;
                asyncResp->res.jsonValue["Id"] = accountName;
                asyncResp->res.jsonValue["UserName"] = accountName;
            },
            "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        std::optional<std::string> newUserName;
        std::optional<std::string> password;
        std::optional<bool> enabled;
        std::optional<std::string> roleId;
        std::optional<bool> locked;
        if (!json_util::readJson(req, res, "UserName", newUserName, "Password",
                                 password, "RoleId", roleId, "Enabled", enabled,
                                 "Locked", locked))
        {
            return;
        }

        const std::string& username = params[0];

        if (!newUserName)
        {
            // If the username isn't being updated, we can update the properties
            // directly
            updateUserProperties(asyncResp, username, password, enabled, roleId,
                                 locked);
            return;
        }
        else
        {
            crow::connections::systemBus->async_method_call(
                [this, asyncResp, username, password(std::move(password)),
                 roleId(std::move(roleId)), enabled(std::move(enabled)),
                 newUser{std::string(*newUserName)}, locked(std::move(locked))](
                    const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                        messages::resourceNotFound(
                            asyncResp->res,
                            "#ManagerAccount.v1_0_3.ManagerAccount", username);
                        return;
                    }

                    updateUserProperties(asyncResp, newUser, password, enabled,
                                         roleId, locked);
                },
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "xyz.openbmc_project.User.Manager", "RenameUser", username,
                *newUserName);
        }
    }

    void updateUserProperties(std::shared_ptr<AsyncResp> asyncResp,
                              const std::string& username,
                              std::optional<std::string> password,
                              std::optional<bool> enabled,
                              std::optional<std::string> roleId,
                              std::optional<bool> locked)
    {
        if (password)
        {
            if (!pamUpdatePassword(username, *password))
            {
                BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                messages::internalError(asyncResp->res);
                return;
            }
        }

        std::string dbusObjectPath = "/xyz/openbmc_project/user/" + username;
        dbus::utility::escapePathForDbus(dbusObjectPath);

        checkDbusPathExists(
            dbusObjectPath,
            [dbusObjectPath(std::move(dbusObjectPath)), username,
             password(std::move(password)), roleId(std::move(roleId)),
             enabled(std::move(enabled)), locked(std::move(locked)),
             asyncResp{std::move(asyncResp)}](int rc) {
                if (!rc)
                {
                    messages::invalidObject(asyncResp->res, username.c_str());
                    return;
                }
                if (enabled)
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "D-Bus responses error: "
                                                 << ec;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            messages::success(asyncResp->res);
                            return;
                        },
                        "xyz.openbmc_project.User.Manager",
                        dbusObjectPath.c_str(),
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.User.Attributes", "UserEnabled",
                        std::variant<bool>{*enabled});
                }

                if (roleId)
                {
                    std::string priv = getRoleIdFromPrivilege(*roleId);
                    if (priv.empty())
                    {
                        messages::propertyValueNotInList(asyncResp->res,
                                                         *roleId, "RoleId");
                        return;
                    }

                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "D-Bus responses error: "
                                                 << ec;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            messages::success(asyncResp->res);
                        },
                        "xyz.openbmc_project.User.Manager",
                        dbusObjectPath.c_str(),
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.User.Attributes", "UserPrivilege",
                        std::variant<std::string>{priv});
                }

                if (locked)
                {
                    // admin can unlock the account which is locked by
                    // successive authentication failures but admin should not
                    // be allowed to lock an account.
                    if (*locked)
                    {
                        messages::propertyValueNotInList(asyncResp->res, "true",
                                                         "Locked");
                        return;
                    }

                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "D-Bus responses error: "
                                                 << ec;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            messages::success(asyncResp->res);
                            return;
                        },
                        "xyz.openbmc_project.User.Manager",
                        dbusObjectPath.c_str(),
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.User.Attributes",
                        "UserLockedForFailedAttempt",
                        sdbusplus::message::variant<bool>{*locked});
                }
            });
    }

    void doDelete(crow::Response& res, const crow::Request& req,
                  const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string userPath = "/xyz/openbmc_project/user/" + params[0];

        crow::connections::systemBus->async_method_call(
            [asyncResp, username{std::move(params[0])}](
                const boost::system::error_code ec) {
                if (ec)
                {
                    messages::resourceNotFound(
                        asyncResp->res, "#ManagerAccount.v1_0_3.ManagerAccount",
                        username);
                    return;
                }

                messages::accountRemoved(asyncResp->res);
            },
            "xyz.openbmc_project.User.Manager", userPath,
            "xyz.openbmc_project.Object.Delete", "Delete");
    }
};

} // namespace redfish
