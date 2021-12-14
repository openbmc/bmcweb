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

#include <app.hpp>
#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <openbmc_dbus_rest.hpp>
#include <persistent_data.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/json_utils.hpp>

#include <variant>

namespace redfish
{

constexpr const char* ldapConfigObjectName =
    "/xyz/openbmc_project/user/ldap/openldap";
constexpr const char* adConfigObject =
    "/xyz/openbmc_project/user/ldap/active_directory";

constexpr const char* ldapRootObject = "/xyz/openbmc_project/user/ldap";
constexpr const char* ldapDbusService = "xyz.openbmc_project.Ldap.Config";
constexpr const char* ldapConfigInterface =
    "xyz.openbmc_project.User.Ldap.Config";
constexpr const char* ldapCreateInterface =
    "xyz.openbmc_project.User.Ldap.Create";
constexpr const char* ldapEnableInterface = "xyz.openbmc_project.Object.Enable";
constexpr const char* ldapPrivMapperInterface =
    "xyz.openbmc_project.User.PrivilegeMapper";
constexpr const char* dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
constexpr const char* propertyInterface = "org.freedesktop.DBus.Properties";
constexpr const char* mapperBusName = "xyz.openbmc_project.ObjectMapper";
constexpr const char* mapperObjectPath = "/xyz/openbmc_project/object_mapper";
constexpr const char* mapperIntf = "xyz.openbmc_project.ObjectMapper";

struct LDAPRoleMapData
{
    std::string groupName;
    std::string privilege;
};

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
    std::vector<std::pair<std::string, LDAPRoleMapData>> groupRoleList;
};

using DbusVariantType = std::variant<bool, int32_t, std::string>;

using DbusInterfaceType = boost::container::flat_map<
    std::string, boost::container::flat_map<std::string, DbusVariantType>>;

using ManagedObjectType =
    std::vector<std::pair<sdbusplus::message::object_path, DbusInterfaceType>>;

using GetObjectType =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

inline std::string getRoleIdFromPrivilege(std::string_view role)
{
    if (role == "priv-admin")
    {
        return "Administrator";
    }
    if (role == "priv-user")
    {
        return "ReadOnly";
    }
    if (role == "priv-operator")
    {
        return "Operator";
    }
    if ((role == "") || (role == "priv-noaccess"))
    {
        return "NoAccess";
    }
    return "";
}
inline std::string getPrivilegeFromRoleId(std::string_view role)
{
    if (role == "Administrator")
    {
        return "priv-admin";
    }
    if (role == "ReadOnly")
    {
        return "priv-user";
    }
    if (role == "Operator")
    {
        return "priv-operator";
    }
    if ((role == "NoAccess") || (role == ""))
    {
        return "priv-noaccess";
    }
    return "";
}

inline void userErrorMessageHandler(
    const sd_bus_error* e, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& newUser, const std::string& username)
{
    if (e == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    const char* errorMessage = e->name;
    if (strcmp(errorMessage,
               "xyz.openbmc_project.User.Common.Error.UserNameExists") == 0)
    {
        messages::resourceAlreadyExists(asyncResp->res,
                                        "#ManagerAccount.v1_4_0.ManagerAccount",
                                        "UserName", newUser);
    }
    else if (strcmp(errorMessage, "xyz.openbmc_project.User.Common.Error."
                                  "UserNameDoesNotExist") == 0)
    {
        messages::resourceNotFound(
            asyncResp->res, "#ManagerAccount.v1_4_0.ManagerAccount", username);
    }
    else if ((strcmp(errorMessage,
                     "xyz.openbmc_project.Common.Error.InvalidArgument") ==
              0) ||
             (strcmp(
                  errorMessage,
                  "xyz.openbmc_project.User.Common.Error.UserNameGroupFail") ==
              0))
    {
        messages::propertyValueFormatError(asyncResp->res, newUser, "UserName");
    }
    else if (strcmp(errorMessage,
                    "xyz.openbmc_project.User.Common.Error.NoResource") == 0)
    {
        messages::createLimitReachedForResource(asyncResp->res);
    }
    else
    {
        messages::internalError(asyncResp->res);
    }

    return;
}

inline void parseLDAPConfigData(nlohmann::json& jsonResponse,
                                const LDAPConfigData& confData,
                                const std::string& ldapType)
{
    std::string service =
        (ldapType == "LDAP") ? "LDAPService" : "ActiveDirectoryService";
    nlohmann::json ldap = {
        {"ServiceEnabled", confData.serviceEnabled},
        {"ServiceAddresses", nlohmann::json::array({confData.uri})},
        {"Authentication",
         {{"AuthenticationType", "UsernameAndPassword"},
          {"Username", confData.bindDN},
          {"Password", nullptr}}},
        {"LDAPService",
         {{"SearchSettings",
           {{"BaseDistinguishedNames",
             nlohmann::json::array({confData.baseDN})},
            {"UsernameAttribute", confData.userNameAttribute},
            {"GroupsAttribute", confData.groupAttribute}}}}},
    };

    jsonResponse[ldapType].update(ldap);

    nlohmann::json& roleMapArray = jsonResponse[ldapType]["RemoteRoleMapping"];
    roleMapArray = nlohmann::json::array();
    for (auto& obj : confData.groupRoleList)
    {
        BMCWEB_LOG_DEBUG << "Pushing the data groupName="
                         << obj.second.groupName << "\n";
        roleMapArray.push_back(
            {nlohmann::json::array({"RemoteGroup", obj.second.groupName}),
             nlohmann::json::array(
                 {"LocalRole", getRoleIdFromPrivilege(obj.second.privilege)})});
    }
}

/**
 *  @brief validates given JSON input and then calls appropriate method to
 * create, to delete or to set Rolemapping object based on the given input.
 *
 */
inline void handleRoleMapPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::pair<std::string, LDAPRoleMapData>>& roleMapObjData,
    const std::string& serverType, const std::vector<nlohmann::json>& input)
{
    for (size_t index = 0; index < input.size(); index++)
    {
        const nlohmann::json& thisJson = input[index];

        if (thisJson.is_null())
        {
            // delete the existing object
            if (index < roleMapObjData.size())
            {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, roleMapObjData, serverType,
                     index](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res
                            .jsonValue[serverType]["RemoteRoleMapping"][index] =
                            nullptr;
                    },
                    ldapDbusService, roleMapObjData[index].first,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            }
            else
            {
                BMCWEB_LOG_ERROR << "Can't delete the object";
                messages::propertyValueTypeError(
                    asyncResp->res,
                    thisJson.dump(2, ' ', true,
                                  nlohmann::json::error_handler_t::replace),
                    "RemoteRoleMapping/" + std::to_string(index));
                return;
            }
        }
        else if (thisJson.empty())
        {
            // Don't do anything for the empty objects,parse next json
            // eg {"RemoteRoleMapping",[{}]}
        }
        else
        {
            // update/create the object
            std::optional<std::string> remoteGroup;
            std::optional<std::string> localRole;

            // This is a copy, but it's required in this case because of how
            // readJson is structured
            nlohmann::json thisJsonCopy = thisJson;
            if (!json_util::readJson(thisJsonCopy, asyncResp->res,
                                     "RemoteGroup", remoteGroup, "LocalRole",
                                     localRole))
            {
                continue;
            }

            // Update existing RoleMapping Object
            if (index < roleMapObjData.size())
            {
                BMCWEB_LOG_DEBUG << "Update Role Map Object";
                // If "RemoteGroup" info is provided
                if (remoteGroup)
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp, roleMapObjData, serverType, index,
                         remoteGroup](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "DBUS response error: "
                                                 << ec;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res
                                .jsonValue[serverType]["RemoteRoleMapping"]
                                          [index]["RemoteGroup"] = *remoteGroup;
                        },
                        ldapDbusService, roleMapObjData[index].first,
                        propertyInterface, "Set",
                        "xyz.openbmc_project.User.PrivilegeMapperEntry",
                        "GroupName",
                        std::variant<std::string>(std::move(*remoteGroup)));
                }

                // If "LocalRole" info is provided
                if (localRole)
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp, roleMapObjData, serverType, index,
                         localRole](const boost::system::error_code ec) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "DBUS response error: "
                                                 << ec;
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res
                                .jsonValue[serverType]["RemoteRoleMapping"]
                                          [index]["LocalRole"] = *localRole;
                        },
                        ldapDbusService, roleMapObjData[index].first,
                        propertyInterface, "Set",
                        "xyz.openbmc_project.User.PrivilegeMapperEntry",
                        "Privilege",
                        std::variant<std::string>(
                            getPrivilegeFromRoleId(std::move(*localRole))));
                }
            }
            // Create a new RoleMapping Object.
            else
            {
                BMCWEB_LOG_DEBUG
                    << "setRoleMappingProperties: Creating new Object";
                std::string pathString =
                    "RemoteRoleMapping/" + std::to_string(index);

                if (!localRole)
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/LocalRole");
                    continue;
                }
                if (!remoteGroup)
                {
                    messages::propertyMissing(asyncResp->res,
                                              pathString + "/RemoteGroup");
                    continue;
                }

                std::string dbusObjectPath;
                if (serverType == "ActiveDirectory")
                {
                    dbusObjectPath = adConfigObject;
                }
                else if (serverType == "LDAP")
                {
                    dbusObjectPath = ldapConfigObjectName;
                }

                BMCWEB_LOG_DEBUG << "Remote Group=" << *remoteGroup
                                 << ",LocalRole=" << *localRole;

                crow::connections::systemBus->async_method_call(
                    [asyncResp, serverType, localRole,
                     remoteGroup](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "DBUS response error: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        nlohmann::json& remoteRoleJson =
                            asyncResp->res
                                .jsonValue[serverType]["RemoteRoleMapping"];
                        remoteRoleJson.push_back(
                            {{"LocalRole", *localRole},
                             {"RemoteGroup", *remoteGroup}});
                    },
                    ldapDbusService, dbusObjectPath, ldapPrivMapperInterface,
                    "Create", *remoteGroup,
                    getPrivilegeFromRoleId(std::move(*localRole)));
            }
        }
    }
}

/**
 * Function that retrieves all properties for LDAP config object
 * into JSON
 */
template <typename CallbackFunc>
inline void getLDAPConfigData(const std::string& ldapType,
                              CallbackFunc&& callback)
{

    const std::array<const char*, 2> interfaces = {ldapEnableInterface,
                                                   ldapConfigInterface};

    crow::connections::systemBus->async_method_call(
        [callback, ldapType](const boost::system::error_code ec,
                             const GetObjectType& resp) {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_ERROR
                    << "DBUS response error during getting of service name: "
                    << ec;
                LDAPConfigData empty{};
                callback(false, empty, ldapType);
                return;
            }
            std::string service = resp.begin()->first;
            crow::connections::systemBus->async_method_call(
                [callback, ldapType](const boost::system::error_code errorCode,
                                     const ManagedObjectType& ldapObjects) {
                    LDAPConfigData confData{};
                    if (errorCode)
                    {
                        callback(false, confData, ldapType);
                        BMCWEB_LOG_ERROR << "D-Bus responses error: "
                                         << errorCode;
                        return;
                    }

                    std::string ldapDbusType;
                    std::string searchString;

                    if (ldapType == "LDAP")
                    {
                        ldapDbusType =
                            "xyz.openbmc_project.User.Ldap.Config.Type.OpenLdap";
                        searchString = "openldap";
                    }
                    else if (ldapType == "ActiveDirectory")
                    {
                        ldapDbusType =
                            "xyz.openbmc_project.User.Ldap.Config.Type.ActiveDirectory";
                        searchString = "active_directory";
                    }
                    else
                    {
                        BMCWEB_LOG_ERROR
                            << "Can't get the DbusType for the given type="
                            << ldapType;
                        callback(false, confData, ldapType);
                        return;
                    }

                    std::string ldapEnableInterfaceStr = ldapEnableInterface;
                    std::string ldapConfigInterfaceStr = ldapConfigInterface;

                    for (const auto& object : ldapObjects)
                    {
                        // let's find the object whose ldap type is equal to the
                        // given type
                        if (object.first.str.find(searchString) ==
                            std::string::npos)
                        {
                            continue;
                        }

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
                                    const std::string* strValue =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (strValue == nullptr)
                                    {
                                        continue;
                                    }
                                    if (property.first == "LDAPServerURI")
                                    {
                                        confData.uri = *strValue;
                                    }
                                    else if (property.first == "LDAPBindDN")
                                    {
                                        confData.bindDN = *strValue;
                                    }
                                    else if (property.first == "LDAPBaseDN")
                                    {
                                        confData.baseDN = *strValue;
                                    }
                                    else if (property.first ==
                                             "LDAPSearchScope")
                                    {
                                        confData.searchScope = *strValue;
                                    }
                                    else if (property.first ==
                                             "GroupNameAttribute")
                                    {
                                        confData.groupAttribute = *strValue;
                                    }
                                    else if (property.first ==
                                             "UserNameAttribute")
                                    {
                                        confData.userNameAttribute = *strValue;
                                    }
                                    else if (property.first == "LDAPType")
                                    {
                                        confData.serverType = *strValue;
                                    }
                                }
                            }
                            else if (
                                interface.first ==
                                "xyz.openbmc_project.User.PrivilegeMapperEntry")
                            {
                                LDAPRoleMapData roleMapData{};
                                for (const auto& property : interface.second)
                                {
                                    const std::string* strValue =
                                        std::get_if<std::string>(
                                            &property.second);

                                    if (strValue == nullptr)
                                    {
                                        continue;
                                    }

                                    if (property.first == "GroupName")
                                    {
                                        roleMapData.groupName = *strValue;
                                    }
                                    else if (property.first == "Privilege")
                                    {
                                        roleMapData.privilege = *strValue;
                                    }
                                }

                                confData.groupRoleList.emplace_back(
                                    object.first.str, roleMapData);
                            }
                        }
                    }
                    callback(true, confData, ldapType);
                },
                service, ldapRootObject, dbusObjManagerIntf,
                "GetManagedObjects");
        },
        mapperBusName, mapperObjectPath, mapperIntf, "GetObject",
        ldapConfigObjectName, interfaces);
}

/**
 * @brief parses the authentication section under the LDAP
 * @param input JSON data
 * @param asyncResp pointer to the JSON response
 * @param userName  userName to be filled from the given JSON.
 * @param password  password to be filled from the given JSON.
 */
inline void parseLDAPAuthenticationJson(
    nlohmann::json input, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::optional<std::string>& username, std::optional<std::string>& password)
{
    std::optional<std::string> authType;

    if (!json_util::readJson(input, asyncResp->res, "AuthenticationType",
                             authType, "Username", username, "Password",
                             password))
    {
        return;
    }
    if (!authType)
    {
        return;
    }
    if (*authType != "UsernameAndPassword")
    {
        messages::propertyValueNotInList(asyncResp->res, *authType,
                                         "AuthenticationType");
        return;
    }
}
/**
 * @brief parses the LDAPService section under the LDAP
 * @param input JSON data
 * @param asyncResp pointer to the JSON response
 * @param baseDNList baseDN to be filled from the given JSON.
 * @param userNameAttribute  userName to be filled from the given JSON.
 * @param groupaAttribute  password to be filled from the given JSON.
 */

inline void
    parseLDAPServiceJson(nlohmann::json input,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         std::optional<std::vector<std::string>>& baseDNList,
                         std::optional<std::string>& userNameAttribute,
                         std::optional<std::string>& groupsAttribute)
{
    std::optional<nlohmann::json> searchSettings;

    if (!json_util::readJson(input, asyncResp->res, "SearchSettings",
                             searchSettings))
    {
        return;
    }
    if (!searchSettings)
    {
        return;
    }
    if (!json_util::readJson(*searchSettings, asyncResp->res,
                             "BaseDistinguishedNames", baseDNList,
                             "UsernameAttribute", userNameAttribute,
                             "GroupsAttribute", groupsAttribute))
    {
        return;
    }
}
/**
 * @brief updates the LDAP server address and updates the
          json response with the new value.
 * @param serviceAddressList address to be updated.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 server(openLDAP/ActiveDirectory)
 */

inline void handleServiceAddressPatch(
    const std::vector<std::string>& serviceAddressList,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& ldapServerElementName,
    const std::string& ldapConfigObject)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, ldapServerElementName,
         serviceAddressList](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "Error Occurred in updating the service address";
                messages::internalError(asyncResp->res);
                return;
            }
            std::vector<std::string> modifiedserviceAddressList = {
                serviceAddressList.front()};
            asyncResp->res
                .jsonValue[ldapServerElementName]["ServiceAddresses"] =
                modifiedserviceAddressList;
            if ((serviceAddressList).size() > 1)
            {
                messages::propertyValueModified(asyncResp->res,
                                                "ServiceAddresses",
                                                serviceAddressList.front());
            }
            BMCWEB_LOG_DEBUG << "Updated the service address";
        },
        ldapDbusService, ldapConfigObject, propertyInterface, "Set",
        ldapConfigInterface, "LDAPServerURI",
        std::variant<std::string>(serviceAddressList.front()));
}
/**
 * @brief updates the LDAP Bind DN and updates the
          json response with the new value.
 * @param username name of the user which needs to be updated.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 server(openLDAP/ActiveDirectory)
 */

inline void
    handleUserNamePatch(const std::string& username,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& ldapServerElementName,
                        const std::string& ldapConfigObject)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, username,
         ldapServerElementName](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Error occurred in updating the username";
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue[ldapServerElementName]["Authentication"]
                                    ["Username"] = username;
            BMCWEB_LOG_DEBUG << "Updated the username";
        },
        ldapDbusService, ldapConfigObject, propertyInterface, "Set",
        ldapConfigInterface, "LDAPBindDN", std::variant<std::string>(username));
}

/**
 * @brief updates the LDAP password
 * @param password : ldap password which needs to be updated.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 *        server(openLDAP/ActiveDirectory)
 */

inline void
    handlePasswordPatch(const std::string& password,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& ldapServerElementName,
                        const std::string& ldapConfigObject)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, password,
         ldapServerElementName](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Error occurred in updating the password";
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue[ldapServerElementName]["Authentication"]
                                    ["Password"] = "";
            BMCWEB_LOG_DEBUG << "Updated the password";
        },
        ldapDbusService, ldapConfigObject, propertyInterface, "Set",
        ldapConfigInterface, "LDAPBindDNPassword",
        std::variant<std::string>(password));
}

/**
 * @brief updates the LDAP BaseDN and updates the
          json response with the new value.
 * @param baseDNList baseDN list which needs to be updated.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 server(openLDAP/ActiveDirectory)
 */

inline void
    handleBaseDNPatch(const std::vector<std::string>& baseDNList,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& ldapServerElementName,
                      const std::string& ldapConfigObject)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, baseDNList,
         ldapServerElementName](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Error Occurred in Updating the base DN";
                messages::internalError(asyncResp->res);
                return;
            }
            auto& serverTypeJson =
                asyncResp->res.jsonValue[ldapServerElementName];
            auto& searchSettingsJson =
                serverTypeJson["LDAPService"]["SearchSettings"];
            std::vector<std::string> modifiedBaseDNList = {baseDNList.front()};
            searchSettingsJson["BaseDistinguishedNames"] = modifiedBaseDNList;
            if (baseDNList.size() > 1)
            {
                messages::propertyValueModified(asyncResp->res,
                                                "BaseDistinguishedNames",
                                                baseDNList.front());
            }
            BMCWEB_LOG_DEBUG << "Updated the base DN";
        },
        ldapDbusService, ldapConfigObject, propertyInterface, "Set",
        ldapConfigInterface, "LDAPBaseDN",
        std::variant<std::string>(baseDNList.front()));
}
/**
 * @brief updates the LDAP user name attribute and updates the
          json response with the new value.
 * @param userNameAttribute attribute to be updated.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 server(openLDAP/ActiveDirectory)
 */

inline void
    handleUserNameAttrPatch(const std::string& userNameAttribute,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& ldapServerElementName,
                            const std::string& ldapConfigObject)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, userNameAttribute,
         ldapServerElementName](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Error Occurred in Updating the "
                                    "username attribute";
                messages::internalError(asyncResp->res);
                return;
            }
            auto& serverTypeJson =
                asyncResp->res.jsonValue[ldapServerElementName];
            auto& searchSettingsJson =
                serverTypeJson["LDAPService"]["SearchSettings"];
            searchSettingsJson["UsernameAttribute"] = userNameAttribute;
            BMCWEB_LOG_DEBUG << "Updated the user name attr.";
        },
        ldapDbusService, ldapConfigObject, propertyInterface, "Set",
        ldapConfigInterface, "UserNameAttribute",
        std::variant<std::string>(userNameAttribute));
}
/**
 * @brief updates the LDAP group attribute and updates the
          json response with the new value.
 * @param groupsAttribute attribute to be updated.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 server(openLDAP/ActiveDirectory)
 */

inline void handleGroupNameAttrPatch(
    const std::string& groupsAttribute,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& ldapServerElementName,
    const std::string& ldapConfigObject)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, groupsAttribute,
         ldapServerElementName](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "Error Occurred in Updating the "
                                    "groupname attribute";
                messages::internalError(asyncResp->res);
                return;
            }
            auto& serverTypeJson =
                asyncResp->res.jsonValue[ldapServerElementName];
            auto& searchSettingsJson =
                serverTypeJson["LDAPService"]["SearchSettings"];
            searchSettingsJson["GroupsAttribute"] = groupsAttribute;
            BMCWEB_LOG_DEBUG << "Updated the groupname attr";
        },
        ldapDbusService, ldapConfigObject, propertyInterface, "Set",
        ldapConfigInterface, "GroupNameAttribute",
        std::variant<std::string>(groupsAttribute));
}
/**
 * @brief updates the LDAP service enable and updates the
          json response with the new value.
 * @param input JSON data.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 server(openLDAP/ActiveDirectory)
 */

inline void handleServiceEnablePatch(
    bool serviceEnabled, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& ldapServerElementName,
    const std::string& ldapConfigObject)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceEnabled,
         ldapServerElementName](const boost::system::error_code ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "Error Occurred in Updating the service enable";
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue[ldapServerElementName]["ServiceEnabled"] =
                serviceEnabled;
            BMCWEB_LOG_DEBUG << "Updated Service enable = " << serviceEnabled;
        },
        ldapDbusService, ldapConfigObject, propertyInterface, "Set",
        ldapEnableInterface, "Enabled", std::variant<bool>(serviceEnabled));
}

inline void
    handleAuthMethodsPatch(nlohmann::json& input,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::optional<bool> basicAuth;
    std::optional<bool> cookie;
    std::optional<bool> sessionToken;
    std::optional<bool> xToken;
    std::optional<bool> tls;

    if (!json_util::readJson(input, asyncResp->res, "BasicAuth", basicAuth,
                             "Cookie", cookie, "SessionToken", sessionToken,
                             "XToken", xToken, "TLS", tls))
    {
        BMCWEB_LOG_ERROR << "Cannot read values from AuthMethod tag";
        return;
    }

    // Make a copy of methods configuration
    persistent_data::AuthConfigMethods authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    if (basicAuth)
    {
#ifndef BMCWEB_ENABLE_BASIC_AUTHENTICATION
        messages::actionNotSupported(
            asyncResp->res,
            "Setting BasicAuth when basic-auth feature is disabled");
        return;
#endif
        authMethodsConfig.basic = *basicAuth;
    }

    if (cookie)
    {
#ifndef BMCWEB_ENABLE_COOKIE_AUTHENTICATION
        messages::actionNotSupported(
            asyncResp->res,
            "Setting Cookie when cookie-auth feature is disabled");
        return;
#endif
        authMethodsConfig.cookie = *cookie;
    }

    if (sessionToken)
    {
#ifndef BMCWEB_ENABLE_SESSION_AUTHENTICATION
        messages::actionNotSupported(
            asyncResp->res,
            "Setting SessionToken when session-auth feature is disabled");
        return;
#endif
        authMethodsConfig.sessionToken = *sessionToken;
    }

    if (xToken)
    {
#ifndef BMCWEB_ENABLE_XTOKEN_AUTHENTICATION
        messages::actionNotSupported(
            asyncResp->res,
            "Setting XToken when xtoken-auth feature is disabled");
        return;
#endif
        authMethodsConfig.xtoken = *xToken;
    }

    if (tls)
    {
#ifndef BMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION
        messages::actionNotSupported(
            asyncResp->res,
            "Setting TLS when mutual-tls-auth feature is disabled");
        return;
#endif
        authMethodsConfig.tls = *tls;
    }

    if (!authMethodsConfig.basic && !authMethodsConfig.cookie &&
        !authMethodsConfig.sessionToken && !authMethodsConfig.xtoken &&
        !authMethodsConfig.tls)
    {
        // Do not allow user to disable everything
        messages::actionNotSupported(asyncResp->res,
                                     "of disabling all available methods");
        return;
    }

    persistent_data::SessionStore::getInstance().updateAuthMethodsConfig(
        authMethodsConfig);
    // Save configuration immediately
    persistent_data::getConfig().writeData();

    messages::success(asyncResp->res);
}

/**
 * @brief Get the required values from the given JSON, validates the
 *        value and create the LDAP config object.
 * @param input JSON data
 * @param asyncResp pointer to the JSON response
 * @param serverType Type of LDAP server(openLDAP/ActiveDirectory)
 */

inline void handleLDAPPatch(nlohmann::json& input,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& serverType)
{
    std::string dbusObjectPath;
    if (serverType == "ActiveDirectory")
    {
        dbusObjectPath = adConfigObject;
    }
    else if (serverType == "LDAP")
    {
        dbusObjectPath = ldapConfigObjectName;
    }
    else
    {
        return;
    }

    std::optional<nlohmann::json> authentication;
    std::optional<nlohmann::json> ldapService;
    std::optional<std::vector<std::string>> serviceAddressList;
    std::optional<bool> serviceEnabled;
    std::optional<std::vector<std::string>> baseDNList;
    std::optional<std::string> userNameAttribute;
    std::optional<std::string> groupsAttribute;
    std::optional<std::string> userName;
    std::optional<std::string> password;
    std::optional<std::vector<nlohmann::json>> remoteRoleMapData;

    if (!json_util::readJson(input, asyncResp->res, "Authentication",
                             authentication, "LDAPService", ldapService,
                             "ServiceAddresses", serviceAddressList,
                             "ServiceEnabled", serviceEnabled,
                             "RemoteRoleMapping", remoteRoleMapData))
    {
        return;
    }

    if (authentication)
    {
        parseLDAPAuthenticationJson(*authentication, asyncResp, userName,
                                    password);
    }
    if (ldapService)
    {
        parseLDAPServiceJson(*ldapService, asyncResp, baseDNList,
                             userNameAttribute, groupsAttribute);
    }
    if (serviceAddressList)
    {
        if ((*serviceAddressList).size() == 0)
        {
            messages::propertyValueNotInList(asyncResp->res, "[]",
                                             "ServiceAddress");
            return;
        }
    }
    if (baseDNList)
    {
        if ((*baseDNList).size() == 0)
        {
            messages::propertyValueNotInList(asyncResp->res, "[]",
                                             "BaseDistinguishedNames");
            return;
        }
    }

    // nothing to update, then return
    if (!userName && !password && !serviceAddressList && !baseDNList &&
        !userNameAttribute && !groupsAttribute && !serviceEnabled &&
        !remoteRoleMapData)
    {
        return;
    }

    // Get the existing resource first then keep modifying
    // whenever any property gets updated.
    getLDAPConfigData(serverType, [asyncResp, userName, password, baseDNList,
                                   userNameAttribute, groupsAttribute,
                                   serviceAddressList, serviceEnabled,
                                   dbusObjectPath, remoteRoleMapData](
                                      bool success,
                                      const LDAPConfigData& confData,
                                      const std::string& serverT) {
        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        parseLDAPConfigData(asyncResp->res.jsonValue, confData, serverT);
        if (confData.serviceEnabled)
        {
            // Disable the service first and update the rest of
            // the properties.
            handleServiceEnablePatch(false, asyncResp, serverT, dbusObjectPath);
        }

        if (serviceAddressList)
        {
            handleServiceAddressPatch(*serviceAddressList, asyncResp, serverT,
                                      dbusObjectPath);
        }
        if (userName)
        {
            handleUserNamePatch(*userName, asyncResp, serverT, dbusObjectPath);
        }
        if (password)
        {
            handlePasswordPatch(*password, asyncResp, serverT, dbusObjectPath);
        }

        if (baseDNList)
        {
            handleBaseDNPatch(*baseDNList, asyncResp, serverT, dbusObjectPath);
        }
        if (userNameAttribute)
        {
            handleUserNameAttrPatch(*userNameAttribute, asyncResp, serverT,
                                    dbusObjectPath);
        }
        if (groupsAttribute)
        {
            handleGroupNameAttrPatch(*groupsAttribute, asyncResp, serverT,
                                     dbusObjectPath);
        }
        if (serviceEnabled)
        {
            // if user has given the value as true then enable
            // the service. if user has given false then no-op
            // as service is already stopped.
            if (*serviceEnabled)
            {
                handleServiceEnablePatch(*serviceEnabled, asyncResp, serverT,
                                         dbusObjectPath);
            }
        }
        else
        {
            // if user has not given the service enabled value
            // then revert it to the same state as it was
            // before.
            handleServiceEnablePatch(confData.serviceEnabled, asyncResp,
                                     serverT, dbusObjectPath);
        }

        if (remoteRoleMapData)
        {
            handleRoleMapPatch(asyncResp, confData.groupRoleList, serverT,
                               *remoteRoleMapData);
        }
    });
}

inline void updateUserProperties(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                                 const std::string& username,
                                 std::optional<std::string> password,
                                 std::optional<bool> enabled,
                                 std::optional<std::string> roleId,
                                 std::optional<bool> locked)
{
    std::string dbusObjectPath = "/xyz/openbmc_project/user/" + username;
    dbus::utility::escapePathForDbus(dbusObjectPath);

    dbus::utility::checkDbusPathExists(
        dbusObjectPath,
        [dbusObjectPath, username, password(std::move(password)),
         roleId(std::move(roleId)), enabled, locked,
         asyncResp{std::move(asyncResp)}](int rc) {
            if (!rc)
            {
                messages::resourceNotFound(
                    asyncResp->res, "#ManagerAccount.v1_4_0.ManagerAccount",
                    username);
                return;
            }

            if (password)
            {
                int retval = pamUpdatePassword(username, *password);

                if (retval == PAM_USER_UNKNOWN)
                {
                    messages::resourceNotFound(
                        asyncResp->res, "#ManagerAccount.v1_4_0.ManagerAccount",
                        username);
                }
                else if (retval == PAM_AUTHTOK_ERR)
                {
                    // If password is invalid
                    messages::propertyValueFormatError(asyncResp->res,
                                                       *password, "Password");
                    BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                }
                else if (retval != PAM_SUCCESS)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            }

            if (enabled)
            {
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        messages::success(asyncResp->res);
                        return;
                    },
                    "xyz.openbmc_project.User.Manager", dbusObjectPath.c_str(),
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.User.Attributes", "UserEnabled",
                    std::variant<bool>{*enabled});
            }

            if (roleId)
            {
                std::string priv = getPrivilegeFromRoleId(*roleId);
                if (priv.empty())
                {
                    messages::propertyValueNotInList(asyncResp->res, *roleId,
                                                     "RoleId");
                    return;
                }
                if (priv == "priv-noaccess")
                {
                    priv = "";
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        messages::success(asyncResp->res);
                    },
                    "xyz.openbmc_project.User.Manager", dbusObjectPath.c_str(),
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.User.Attributes", "UserPrivilege",
                    std::variant<std::string>{priv});
            }

            if (locked)
            {
                // admin can unlock the account which is locked by
                // successive authentication failures but admin should
                // not be allowed to lock an account.
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
                            BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        messages::success(asyncResp->res);
                        return;
                    },
                    "xyz.openbmc_project.User.Manager", dbusObjectPath.c_str(),
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.User.Attributes",
                    "UserLockedForFailedAttempt", std::variant<bool>{*locked});
            }
        });
}

inline void requestAccountServiceRoutes(App& app)
{

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/")
        .privileges(redfish::privileges::getAccountService)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request& req,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp)
                                               -> void {
            const persistent_data::AuthConfigMethods& authMethodsConfig =
                persistent_data::SessionStore::getInstance()
                    .getAuthMethodsConfig();

            asyncResp->res.jsonValue = {
                {"@odata.id", "/redfish/v1/AccountService"},
                {"@odata.type", "#AccountService."
                                "v1_5_0.AccountService"},
                {"Id", "AccountService"},
                {"Name", "Account Service"},
                {"Description", "Account Service"},
                {"ServiceEnabled", true},
                {"MaxPasswordLength", 20},
                {"Accounts",
                 {{"@odata.id", "/redfish/v1/AccountService/Accounts"}}},
                {"Roles", {{"@odata.id", "/redfish/v1/AccountService/Roles"}}},
                {"Oem",
                 {{"OpenBMC",
                   {{"@odata.type", "#OemAccountService.v1_0_0.AccountService"},
                    {"@odata.id", "/redfish/v1/AccountService#/Oem/OpenBMC"},
                    {"AuthMethods",
                     {
                         {"BasicAuth", authMethodsConfig.basic},
                         {"SessionToken", authMethodsConfig.sessionToken},
                         {"XToken", authMethodsConfig.xtoken},
                         {"Cookie", authMethodsConfig.cookie},
                         {"TLS", authMethodsConfig.tls},
                     }}}}}}};
            // /redfish/v1/AccountService/LDAP/Certificates is something only
            // ConfigureManager can access then only display when the user has
            // permissions ConfigureManager
            Privileges effectiveUserPrivileges =
                redfish::getUserPrivileges(req.userRole);

            if (isOperationAllowedWithPrivileges({{"ConfigureManager"}},
                                                 effectiveUserPrivileges))
            {
                asyncResp->res.jsonValue["LDAP"] = {
                    {"Certificates",
                     {{"@odata.id",
                       "/redfish/v1/AccountService/LDAP/Certificates"}}}};
            }
            crow::connections::systemBus->async_method_call(
                [asyncResp](
                    const boost::system::error_code ec,
                    const std::vector<
                        std::pair<std::string,
                                  std::variant<uint32_t, uint16_t, uint8_t>>>&
                        propertiesList) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    BMCWEB_LOG_DEBUG << "Got " << propertiesList.size()
                                     << "properties for AccountService";
                    for (const std::pair<std::string,
                                         std::variant<uint32_t, uint16_t,
                                                      uint8_t>>& property :
                         propertiesList)
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
                                asyncResp->res
                                    .jsonValue["AccountLockoutDuration"] =
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
                                    .jsonValue["AccountLockoutThreshold"] =
                                    *value;
                            }
                        }
                    }
                },
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "org.freedesktop.DBus.Properties", "GetAll",
                "xyz.openbmc_project.User.AccountPolicy");

            auto callback = [asyncResp](bool success, LDAPConfigData& confData,
                                        const std::string& ldapType) {
                if (!success)
                {
                    return;
                }
                parseLDAPConfigData(asyncResp->res.jsonValue, confData,
                                    ldapType);
            };

            getLDAPConfigData("LDAP", callback);
            getLDAPConfigData("ActiveDirectory", callback);
        });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/")
        .privileges(redfish::privileges::getAccountService)
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) -> void {
                std::optional<uint32_t> unlockTimeout;
                std::optional<uint16_t> lockoutThreshold;
                std::optional<uint16_t> minPasswordLength;
                std::optional<uint16_t> maxPasswordLength;
                std::optional<nlohmann::json> ldapObject;
                std::optional<nlohmann::json> activeDirectoryObject;
                std::optional<nlohmann::json> oemObject;

                if (!json_util::readJson(
                        req, asyncResp->res, "AccountLockoutDuration",
                        unlockTimeout, "AccountLockoutThreshold",
                        lockoutThreshold, "MaxPasswordLength",
                        maxPasswordLength, "MinPasswordLength",
                        minPasswordLength, "LDAP", ldapObject,
                        "ActiveDirectory", activeDirectoryObject, "Oem",
                        oemObject))
                {
                    return;
                }

                if (minPasswordLength)
                {
                    messages::propertyNotWritable(asyncResp->res,
                                                  "MinPasswordLength");
                }

                if (maxPasswordLength)
                {
                    messages::propertyNotWritable(asyncResp->res,
                                                  "MaxPasswordLength");
                }

                if (ldapObject)
                {
                    handleLDAPPatch(*ldapObject, asyncResp, "LDAP");
                }

                if (std::optional<nlohmann::json> oemOpenBMCObject;
                    oemObject &&
                    json_util::readJson(*oemObject, asyncResp->res, "OpenBMC",
                                        oemOpenBMCObject))
                {
                    if (std::optional<nlohmann::json> authMethodsObject;
                        oemOpenBMCObject &&
                        json_util::readJson(*oemOpenBMCObject, asyncResp->res,
                                            "AuthMethods", authMethodsObject))
                    {
                        if (authMethodsObject)
                        {
                            handleAuthMethodsPatch(*authMethodsObject,
                                                   asyncResp);
                        }
                    }
                }

                if (activeDirectoryObject)
                {
                    handleLDAPPatch(*activeDirectoryObject, asyncResp,
                                    "ActiveDirectory");
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
                        "xyz.openbmc_project.User.Manager",
                        "/xyz/openbmc_project/user",
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.User.AccountPolicy",
                        "AccountUnlockTimeout",
                        std::variant<uint32_t>(*unlockTimeout));
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
                        "xyz.openbmc_project.User.Manager",
                        "/xyz/openbmc_project/user",
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.User.AccountPolicy",
                        "MaxLoginAttemptBeforeLockout",
                        std::variant<uint16_t>(*lockoutThreshold));
                }
            });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
        .privileges(redfish::privileges::getManagerAccountCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) -> void {
                asyncResp->res.jsonValue = {
                    {"@odata.id", "/redfish/v1/AccountService/Accounts"},
                    {"@odata.type", "#ManagerAccountCollection."
                                    "ManagerAccountCollection"},
                    {"Name", "Accounts Collection"},
                    {"Description", "BMC User Accounts"}};

                Privileges effectiveUserPrivileges =
                    redfish::getUserPrivileges(req.userRole);

                std::string thisUser;
                if (req.session)
                {
                    thisUser = req.session->username;
                }
                crow::connections::systemBus->async_method_call(
                    [asyncResp, thisUser, effectiveUserPrivileges](
                        const boost::system::error_code ec,
                        const ManagedObjectType& users) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        bool userCanSeeAllAccounts =
                            effectiveUserPrivileges.isSupersetOf(
                                {"ConfigureUsers"});

                        bool userCanSeeSelf =
                            effectiveUserPrivileges.isSupersetOf(
                                {"ConfigureSelf"});

                        nlohmann::json& memberArray =
                            asyncResp->res.jsonValue["Members"];
                        memberArray = nlohmann::json::array();

                        for (auto& userpath : users)
                        {
                            std::string user = userpath.first.filename();
                            if (user.empty())
                            {
                                messages::internalError(asyncResp->res);
                                BMCWEB_LOG_ERROR << "Invalid firmware ID";

                                return;
                            }

                            // As clarified by Redfish here:
                            // https://redfishforum.com/thread/281/manageraccountcollection-change-allows-account-enumeration
                            // Users without ConfigureUsers, only see their own
                            // account. Users with ConfigureUsers, see all
                            // accounts.
                            if (userCanSeeAllAccounts ||
                                (thisUser == user && userCanSeeSelf))
                            {
                                memberArray.push_back(
                                    {{"@odata.id",
                                      "/redfish/v1/AccountService/Accounts/" +
                                          user}});
                            }
                        }
                        asyncResp->res.jsonValue["Members@odata.count"] =
                            memberArray.size();
                    },
                    "xyz.openbmc_project.User.Manager",
                    "/xyz/openbmc_project/user",
                    "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
            });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
        .privileges(redfish::privileges::postManagerAccountCollection)
        .methods(boost::beast::http::verb::post)([](const crow::Request& req,
                                                    const std::shared_ptr<
                                                        bmcweb::AsyncResp>&
                                                        asyncResp) -> void {
            std::string username;
            std::string password;
            std::optional<std::string> roleId("User");
            std::optional<bool> enabled = true;
            if (!json_util::readJson(req, asyncResp->res, "UserName", username,
                                     "Password", password, "RoleId", roleId,
                                     "Enabled", enabled))
            {
                return;
            }

            std::string priv = getPrivilegeFromRoleId(*roleId);
            if (priv.empty())
            {
                messages::propertyValueNotInList(asyncResp->res, *roleId,
                                                 "RoleId");
                return;
            }
            // TODO: Following override will be reverted once support in
            // phosphor-user-manager is added. In order to avoid dependency
            // issues, this is added in bmcweb, which will removed, once
            // phosphor-user-manager supports priv-noaccess.
            if (priv == "priv-noaccess")
            {
                roleId = "";
            }
            else
            {
                roleId = priv;
            }

            // Reading AllGroups property
            crow::connections::systemBus->async_method_call(
                [asyncResp, username, password{std::move(password)}, roleId,
                 enabled](
                    const boost::system::error_code ec,
                    const std::variant<std::vector<std::string>>& allGroups) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "ERROR with async_method_call";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    const std::vector<std::string>* allGroupsList =
                        std::get_if<std::vector<std::string>>(&allGroups);

                    if (allGroupsList == nullptr || allGroupsList->empty())
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    crow::connections::systemBus->async_method_call(
                        [asyncResp, username,
                         password](const boost::system::error_code ec2,
                                   sdbusplus::message::message& m) {
                            if (ec2)
                            {
                                userErrorMessageHandler(
                                    m.get_error(), asyncResp, username, "");
                                return;
                            }

                            if (pamUpdatePassword(username, password) !=
                                PAM_SUCCESS)
                            {
                                // At this point we have a user that's been
                                // created, but the password set
                                // failed.Something is wrong, so delete the user
                                // that we've already created
                                crow::connections::systemBus->async_method_call(
                                    [asyncResp, password](
                                        const boost::system::error_code ec3) {
                                        if (ec3)
                                        {
                                            messages::internalError(
                                                asyncResp->res);
                                            return;
                                        }

                                        // If password is invalid
                                        messages::propertyValueFormatError(
                                            asyncResp->res, password,
                                            "Password");
                                    },
                                    "xyz.openbmc_project.User.Manager",
                                    "/xyz/openbmc_project/user/" + username,
                                    "xyz.openbmc_project.Object.Delete",
                                    "Delete");

                                BMCWEB_LOG_ERROR << "pamUpdatePassword Failed";
                                return;
                            }

                            messages::created(asyncResp->res);
                            asyncResp->res.addHeader(
                                "Location",
                                "/redfish/v1/AccountService/Accounts/" +
                                    username);
                        },
                        "xyz.openbmc_project.User.Manager",
                        "/xyz/openbmc_project/user",
                        "xyz.openbmc_project.User.Manager", "CreateUser",
                        username, *allGroupsList, *roleId, *enabled);
                },
                "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.User.Manager", "AllGroups");
        });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        .privileges(redfish::privileges::getManagerAccount)
        .methods(
            boost::beast::http::verb::
                get)([]([[maybe_unused]] const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& accountName) -> void {
#ifndef BMCWEB_INSECURE_DISABLE_AUTHENTICATION
            if (req.session->username != accountName)
            {
                // At this point we've determined that the user is trying to
                // modify a user that isn't them.  We need to verify that they
                // have permissions to modify other users, so re-run the auth
                // check with the same permissions, minus ConfigureSelf.
                Privileges effectiveUserPrivileges =
                    redfish::getUserPrivileges(req.userRole);
                Privileges requiredPermissionsToChangeNonSelf = {
                    "ConfigureUsers", "ConfigureManager"};
                if (!effectiveUserPrivileges.isSupersetOf(
                        requiredPermissionsToChangeNonSelf))
                {
                    BMCWEB_LOG_DEBUG << "GET Account denied access";
                    messages::insufficientPrivilege(asyncResp->res);
                    return;
                }
            }
#endif // BMCWEB_INSECURE_DISABLE_AUTHENTICATION

            crow::connections::systemBus->async_method_call(
                [asyncResp, accountName](const boost::system::error_code ec,
                                         const ManagedObjectType& users) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    auto userIt = users.begin();

                    for (; userIt != users.end(); userIt++)
                    {
                        if (boost::ends_with(userIt->first.str,
                                             "/" + accountName))
                        {
                            break;
                        }
                    }
                    if (userIt == users.end())
                    {
                        messages::resourceNotFound(
                            asyncResp->res, "ManagerAccount", accountName);
                        return;
                    }

                    asyncResp->res.jsonValue = {
                        {"@odata.type",
                         "#ManagerAccount.v1_4_0.ManagerAccount"},
                        {"Name", "User Account"},
                        {"Description", "User Account"},
                        {"Password", nullptr},
                        {"AccountTypes", {"Redfish"}}};

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
                                        "false"}; // can only unlock accounts
                                }
                                else if (property.first == "UserPrivilege")
                                {
                                    const std::string* userPrivPtr =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (userPrivPtr == nullptr)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "UserPrivilege wasn't a "
                                               "string";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    std::string role =
                                        getRoleIdFromPrivilege(*userPrivPtr);
                                    if (role.empty())
                                    {
                                        BMCWEB_LOG_ERROR << "Invalid user role";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    asyncResp->res.jsonValue["RoleId"] = role;

                                    asyncResp->res.jsonValue["Links"]["Role"] =
                                        {{"@odata.id",
                                          "/redfish/v1/AccountService/Roles/" +
                                              role}};
                                }
                                else if (property.first ==
                                         "UserPasswordExpired")
                                {
                                    const bool* userPasswordExpired =
                                        std::get_if<bool>(&property.second);
                                    if (userPasswordExpired == nullptr)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "UserPasswordExpired wasn't a bool";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    asyncResp->res
                                        .jsonValue["PasswordChangeRequired"] =
                                        *userPasswordExpired;
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
        });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        // TODO this privilege should be using the generated endpoints, but
        // because of the special handling of ConfigureSelf, it's not able to
        // yet
        .privileges({{"ConfigureUsers"}, {"ConfigureSelf"}})
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& username) -> void {
                std::optional<std::string> newUserName;
                std::optional<std::string> password;
                std::optional<bool> enabled;
                std::optional<std::string> roleId;
                std::optional<bool> locked;

                Privileges effectiveUserPrivileges =
                    redfish::getUserPrivileges(req.userRole);
                Privileges configureUsers = {"ConfigureUsers"};
                bool userHasConfigureUsers =
                    effectiveUserPrivileges.isSupersetOf(configureUsers);
                if (userHasConfigureUsers)
                {
                    // Users with ConfigureUsers can modify for all users
                    if (!json_util::readJson(req, asyncResp->res, "UserName",
                                             newUserName, "Password", password,
                                             "RoleId", roleId, "Enabled",
                                             enabled, "Locked", locked))
                    {
                        return;
                    }
                }
                else
                {
#ifndef BMCWEB_INSECURE_DISABLE_AUTHENTICATION
                    // ConfigureSelf accounts can only modify their own account
                    if (username != req.session->username)
                    {
                        messages::insufficientPrivilege(asyncResp->res);
                        return;
                    }
#endif //# BMCWEB_INSECURE_DISABLE_AUTHENTICATION
                    // ConfigureSelf accounts can only modify their password
                    if (!json_util::readJson(req, asyncResp->res, "Password",
                                             password))
                    {
                        return;
                    }
                }

                // if user name is not provided in the patch method or if it
                // matches the user name in the URI, then we are treating it as
                // updating user properties other then username. If username
                // provided doesn't match the URI, then we are treating this as
                // user rename request.
                if (!newUserName || (newUserName.value() == username))
                {
                    updateUserProperties(asyncResp, username, password, enabled,
                                         roleId, locked);
                    return;
                }
                crow::connections::systemBus->async_method_call(
                    [asyncResp, username, password(std::move(password)),
                     roleId(std::move(roleId)), enabled,
                     newUser{std::string(*newUserName)},
                     locked](const boost::system::error_code ec,
                             sdbusplus::message::message& m) {
                        if (ec)
                        {
                            userErrorMessageHandler(m.get_error(), asyncResp,
                                                    newUser, username);
                            return;
                        }

                        updateUserProperties(asyncResp, newUser, password,
                                             enabled, roleId, locked);
                    },
                    "xyz.openbmc_project.User.Manager",
                    "/xyz/openbmc_project/user",
                    "xyz.openbmc_project.User.Manager", "RenameUser", username,
                    *newUserName);
            });

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        .privileges(redfish::privileges::deleteManagerAccount)
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& username) -> void {
                const std::string userPath =
                    "/xyz/openbmc_project/user/" + username;

                crow::connections::systemBus->async_method_call(
                    [asyncResp, username](const boost::system::error_code ec) {
                        if (ec)
                        {
                            messages::resourceNotFound(
                                asyncResp->res,
                                "#ManagerAccount.v1_4_0.ManagerAccount",
                                username);
                            return;
                        }

                        messages::accountRemoved(asyncResp->res);
                    },
                    "xyz.openbmc_project.User.Manager", userPath,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            });
}

} // namespace redfish
