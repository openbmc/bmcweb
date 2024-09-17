/*
Copyright (c) 2018 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once

#include "app.hpp"
#include "boost_formatters.hpp"
#include "certificate_service.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/account_service.hpp"
#include "persistent_data.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sessions.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

constexpr const char* ldapConfigObjectName =
    "/xyz/openbmc_project/user/ldap/openldap";
constexpr const char* adConfigObject =
    "/xyz/openbmc_project/user/ldap/active_directory";

constexpr const char* rootUserDbusPath = "/xyz/openbmc_project/user/";
constexpr const char* ldapRootObject = "/xyz/openbmc_project/user/ldap";
constexpr const char* ldapDbusService = "xyz.openbmc_project.Ldap.Config";
constexpr const char* ldapConfigInterface =
    "xyz.openbmc_project.User.Ldap.Config";
constexpr const char* ldapCreateInterface =
    "xyz.openbmc_project.User.Ldap.Create";
constexpr const char* ldapEnableInterface = "xyz.openbmc_project.Object.Enable";
constexpr const char* ldapPrivMapperInterface =
    "xyz.openbmc_project.User.PrivilegeMapper";

struct LDAPRoleMapData
{
    std::string groupName;
    std::string privilege;
};

struct LDAPConfigData
{
    std::string uri;
    std::string bindDN;
    std::string baseDN;
    std::string searchScope;
    std::string serverType;
    bool serviceEnabled = false;
    std::string userNameAttribute;
    std::string groupAttribute;
    std::vector<std::pair<std::string, LDAPRoleMapData>> groupRoleList;
};

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
    return "";
}

/**
 * @brief Maps user group names retrieved from D-Bus object to
 * Account Types.
 *
 * @param[in] userGroups List of User groups
 * @param[out] res AccountTypes populated
 *
 * @return true in case of success, false if UserGroups contains
 * invalid group name(s).
 */
inline bool translateUserGroup(const std::vector<std::string>& userGroups,
                               crow::Response& res)
{
    std::vector<std::string> accountTypes;
    for (const auto& userGroup : userGroups)
    {
        if (userGroup == "redfish")
        {
            accountTypes.emplace_back("Redfish");
            accountTypes.emplace_back("WebUI");
        }
        else if (userGroup == "ipmi")
        {
            accountTypes.emplace_back("IPMI");
        }
        else if (userGroup == "ssh")
        {
            accountTypes.emplace_back("ManagerConsole");
        }
        else if (userGroup == "hostconsole")
        {
            // The hostconsole group controls who can access the host console
            // port via ssh and websocket.
            accountTypes.emplace_back("HostConsole");
        }
        else if (userGroup == "web")
        {
            // 'web' is one of the valid groups in the UserGroups property of
            // the user account in the D-Bus object. This group is currently not
            // doing anything, and is considered to be equivalent to 'redfish'.
            // 'redfish' user group is mapped to 'Redfish'and 'WebUI'
            // AccountTypes, so do nothing here...
        }
        else
        {
            // Invalid user group name. Caller throws an exception.
            return false;
        }
    }

    res.jsonValue["AccountTypes"] = std::move(accountTypes);
    return true;
}

/**
 * @brief Builds User Groups from the Account Types
 *
 * @param[in] asyncResp Async Response
 * @param[in] accountTypes List of Account Types
 * @param[out] userGroups List of User Groups mapped from Account Types
 *
 * @return true if Account Types mapped to User Groups, false otherwise.
 */
inline bool getUserGroupFromAccountType(
    crow::Response& res, const std::vector<std::string>& accountTypes,
    std::vector<std::string>& userGroups)
{
    // Need both Redfish and WebUI Account Types to map to 'redfish' User Group
    bool redfishType = false;
    bool webUIType = false;

    for (const auto& accountType : accountTypes)
    {
        if (accountType == "Redfish")
        {
            redfishType = true;
        }
        else if (accountType == "WebUI")
        {
            webUIType = true;
        }
        else if (accountType == "IPMI")
        {
            userGroups.emplace_back("ipmi");
        }
        else if (accountType == "HostConsole")
        {
            userGroups.emplace_back("hostconsole");
        }
        else if (accountType == "ManagerConsole")
        {
            userGroups.emplace_back("ssh");
        }
        else
        {
            // Invalid Account Type
            messages::propertyValueNotInList(res, "AccountTypes", accountType);
            return false;
        }
    }

    // Both  Redfish and WebUI Account Types are needed to PATCH
    if (redfishType ^ webUIType)
    {
        BMCWEB_LOG_ERROR(
            "Missing Redfish or WebUI Account Type to set redfish User Group");
        messages::strictAccountTypes(res, "AccountTypes");
        return false;
    }

    if (redfishType && webUIType)
    {
        userGroups.emplace_back("redfish");
    }

    return true;
}

/**
 * @brief Sets UserGroups property of the user based on the Account Types
 *
 * @param[in] accountTypes List of User Account Types
 * @param[in] asyncResp Async Response
 * @param[in] dbusObjectPath D-Bus Object Path
 * @param[in] userSelf true if User is updating OWN Account Types
 */
inline void
    patchAccountTypes(const std::vector<std::string>& accountTypes,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& dbusObjectPath, bool userSelf)
{
    // Check if User is disabling own Redfish Account Type
    if (userSelf &&
        (accountTypes.cend() ==
         std::find(accountTypes.cbegin(), accountTypes.cend(), "Redfish")))
    {
        BMCWEB_LOG_ERROR(
            "User disabling OWN Redfish Account Type is not allowed");
        messages::strictAccountTypes(asyncResp->res, "AccountTypes");
        return;
    }

    std::vector<std::string> updatedUserGroups;
    if (!getUserGroupFromAccountType(asyncResp->res, accountTypes,
                                     updatedUserGroups))
    {
        // Problem in mapping Account Types to User Groups, Error already
        // logged.
        return;
    }
    setDbusProperty(asyncResp, "AccountTypes",
                    "xyz.openbmc_project.User.Manager", dbusObjectPath,
                    "xyz.openbmc_project.User.Attributes", "UserGroups",
                    updatedUserGroups);
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
        messages::resourceAlreadyExists(asyncResp->res, "ManagerAccount",
                                        "UserName", newUser);
    }
    else if (strcmp(errorMessage, "xyz.openbmc_project.User.Common.Error."
                                  "UserNameDoesNotExist") == 0)
    {
        messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
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
        BMCWEB_LOG_ERROR("DBUS response error {}", errorMessage);
        messages::internalError(asyncResp->res);
    }
}

inline void parseLDAPConfigData(nlohmann::json& jsonResponse,
                                const LDAPConfigData& confData,
                                const std::string& ldapType)
{
    nlohmann::json::object_t ldap;
    ldap["ServiceEnabled"] = confData.serviceEnabled;
    nlohmann::json::array_t serviceAddresses;
    serviceAddresses.emplace_back(confData.uri);
    ldap["ServiceAddresses"] = std::move(serviceAddresses);

    nlohmann::json::object_t authentication;
    authentication["AuthenticationType"] =
        account_service::AuthenticationTypes::UsernameAndPassword;
    authentication["Username"] = confData.bindDN;
    authentication["Password"] = nullptr;
    ldap["Authentication"] = std::move(authentication);

    nlohmann::json::object_t ldapService;
    nlohmann::json::object_t searchSettings;
    nlohmann::json::array_t baseDistinguishedNames;
    baseDistinguishedNames.emplace_back(confData.baseDN);

    searchSettings["BaseDistinguishedNames"] =
        std::move(baseDistinguishedNames);
    searchSettings["UsernameAttribute"] = confData.userNameAttribute;
    searchSettings["GroupsAttribute"] = confData.groupAttribute;
    ldapService["SearchSettings"] = std::move(searchSettings);
    ldap["LDAPService"] = std::move(ldapService);

    nlohmann::json::array_t roleMapArray;
    for (const auto& obj : confData.groupRoleList)
    {
        BMCWEB_LOG_DEBUG("Pushing the data groupName={}", obj.second.groupName);

        nlohmann::json::object_t remoteGroup;
        remoteGroup["RemoteGroup"] = obj.second.groupName;
        remoteGroup["LocalRole"] = getRoleIdFromPrivilege(obj.second.privilege);
        roleMapArray.emplace_back(std::move(remoteGroup));
    }

    ldap["RemoteRoleMapping"] = std::move(roleMapArray);

    jsonResponse[ldapType].update(ldap);
}

/**
 *  @brief validates given JSON input and then calls appropriate method to
 * create, to delete or to set Rolemapping object based on the given input.
 *
 */
inline void handleRoleMapPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::pair<std::string, LDAPRoleMapData>>& roleMapObjData,
    const std::string& serverType,
    std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>>& input)
{
    for (size_t index = 0; index < input.size(); index++)
    {
        std::variant<nlohmann::json::object_t, std::nullptr_t>& thisJson =
            input[index];
        nlohmann::json::object_t* obj =
            std::get_if<nlohmann::json::object_t>(&thisJson);
        if (obj == nullptr)
        {
            // delete the existing object
            if (index < roleMapObjData.size())
            {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, roleMapObjData, serverType,
                     index](const boost::system::error_code& ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
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
                BMCWEB_LOG_ERROR("Can't delete the object");
                messages::propertyValueTypeError(
                    asyncResp->res, "null",
                    "RemoteRoleMapping/" + std::to_string(index));
                return;
            }
        }
        else if (obj->empty())
        {
            // Don't do anything for the empty objects,parse next json
            // eg {"RemoteRoleMapping",[{}]}
        }
        else
        {
            // update/create the object
            std::optional<std::string> remoteGroup;
            std::optional<std::string> localRole;

            if (!json_util::readJsonObject( //
                    *obj, asyncResp->res, //
                    "LocalRole", localRole, //
                    "RemoteGroup", remoteGroup //
                    ))
            {
                continue;
            }

            // Update existing RoleMapping Object
            if (index < roleMapObjData.size())
            {
                BMCWEB_LOG_DEBUG("Update Role Map Object");
                // If "RemoteGroup" info is provided
                if (remoteGroup)
                {
                    setDbusProperty(
                        asyncResp,
                        std::format("RemoteRoleMapping/{}/RemoteGroup", index),
                        ldapDbusService, roleMapObjData[index].first,
                        "xyz.openbmc_project.User.PrivilegeMapperEntry",
                        "GroupName", *remoteGroup);
                }

                // If "LocalRole" info is provided
                if (localRole)
                {
                    std::string priv = getPrivilegeFromRoleId(*localRole);
                    if (priv.empty())
                    {
                        messages::propertyValueNotInList(
                            asyncResp->res, *localRole,
                            std::format("RemoteRoleMapping/{}/LocalRole",
                                        index));
                        return;
                    }
                    setDbusProperty(
                        asyncResp,
                        std::format("RemoteRoleMapping/{}/LocalRole", index),
                        ldapDbusService, roleMapObjData[index].first,
                        "xyz.openbmc_project.User.PrivilegeMapperEntry",
                        "Privilege", priv);
                }
            }
            // Create a new RoleMapping Object.
            else
            {
                BMCWEB_LOG_DEBUG(
                    "setRoleMappingProperties: Creating new Object");
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

                BMCWEB_LOG_DEBUG("Remote Group={},LocalRole={}", *remoteGroup,
                                 *localRole);

                crow::connections::systemBus->async_method_call(
                    [asyncResp, serverType, localRole,
                     remoteGroup](const boost::system::error_code& ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        nlohmann::json& remoteRoleJson =
                            asyncResp->res
                                .jsonValue[serverType]["RemoteRoleMapping"];
                        nlohmann::json::object_t roleMapEntry;
                        roleMapEntry["LocalRole"] = *localRole;
                        roleMapEntry["RemoteGroup"] = *remoteGroup;
                        remoteRoleJson.emplace_back(std::move(roleMapEntry));
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
inline void
    getLDAPConfigData(const std::string& ldapType, CallbackFunc&& callback)
{
    constexpr std::array<std::string_view, 2> interfaces = {
        ldapEnableInterface, ldapConfigInterface};

    dbus::utility::getDbusObject(
        ldapConfigObjectName, interfaces,
        [callback = std::forward<CallbackFunc>(callback),
         ldapType](const boost::system::error_code& ec,
                   const dbus::utility::MapperGetObject& resp) mutable {
            if (ec || resp.empty())
            {
                BMCWEB_LOG_WARNING(
                    "DBUS response error during getting of service name: {}",
                    ec);
                LDAPConfigData empty{};
                callback(false, empty, ldapType);
                return;
            }
            std::string service = resp.begin()->first;
            sdbusplus::message::object_path path(ldapRootObject);
            dbus::utility::getManagedObjects(
                service, path,
                [callback, ldapType](const boost::system::error_code& ec2,
                                     const dbus::utility::ManagedObjectType&
                                         ldapObjects) mutable {
                    LDAPConfigData confData{};
                    if (ec2)
                    {
                        callback(false, confData, ldapType);
                        BMCWEB_LOG_WARNING("D-Bus responses error: {}", ec2);
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
                        BMCWEB_LOG_ERROR(
                            "Can't get the DbusType for the given type={}",
                            ldapType);
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
                });
        });
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
    setDbusProperty(asyncResp, ldapServerElementName + "/ServiceAddress",
                    ldapDbusService, ldapConfigObject, ldapConfigInterface,
                    "LDAPServerURI", serviceAddressList.front());
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
    setDbusProperty(asyncResp,
                    ldapServerElementName + "/Authentication/Username",
                    ldapDbusService, ldapConfigObject, ldapConfigInterface,
                    "LDAPBindDN", username);
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
    setDbusProperty(asyncResp,
                    ldapServerElementName + "/Authentication/Password",
                    ldapDbusService, ldapConfigObject, ldapConfigInterface,
                    "LDAPBindDNPassword", password);
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
    setDbusProperty(asyncResp,
                    ldapServerElementName +
                        "/LDAPService/SearchSettings/BaseDistinguishedNames",
                    ldapDbusService, ldapConfigObject, ldapConfigInterface,
                    "LDAPBaseDN", baseDNList.front());
}
/**
 * @brief updates the LDAP user name attribute and updates the
          json response with the new value.
 * @param userNameAttribute attribute to be updated.
 * @param asyncResp pointer to the JSON response
 * @param ldapServerElementName Type of LDAP
 server(openLDAP/ActiveDirectory)
 */

inline void handleUserNameAttrPatch(
    const std::string& userNameAttribute,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& ldapServerElementName,
    const std::string& ldapConfigObject)
{
    setDbusProperty(
        asyncResp,
        ldapServerElementName + "LDAPService/SearchSettings/UsernameAttribute",
        ldapDbusService, ldapConfigObject, ldapConfigInterface,
        "UserNameAttribute", userNameAttribute);
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
    setDbusProperty(
        asyncResp,
        ldapServerElementName + "/LDAPService/SearchSettings/GroupsAttribute",
        ldapDbusService, ldapConfigObject, ldapConfigInterface,
        "GroupNameAttribute", groupsAttribute);
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
    setDbusProperty(asyncResp, ldapServerElementName + "/ServiceEnabled",
                    ldapDbusService, ldapConfigObject, ldapEnableInterface,
                    "Enabled", serviceEnabled);
}

struct AuthMethods
{
    std::optional<bool> basicAuth;
    std::optional<bool> cookie;
    std::optional<bool> sessionToken;
    std::optional<bool> xToken;
    std::optional<bool> tls;
};

inline void
    handleAuthMethodsPatch(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const AuthMethods& auth)
{
    persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    if (auth.basicAuth)
    {
        if constexpr (!BMCWEB_BASIC_AUTH)
        {
            messages::actionNotSupported(
                asyncResp->res,
                "Setting BasicAuth when basic-auth feature is disabled");
            return;
        }

        authMethodsConfig.basic = *auth.basicAuth;
    }

    if (auth.cookie)
    {
        if constexpr (!BMCWEB_COOKIE_AUTH)
        {
            messages::actionNotSupported(
                asyncResp->res,
                "Setting Cookie when cookie-auth feature is disabled");
            return;
        }
        authMethodsConfig.cookie = *auth.cookie;
    }

    if (auth.sessionToken)
    {
        if constexpr (!BMCWEB_SESSION_AUTH)
        {
            messages::actionNotSupported(
                asyncResp->res,
                "Setting SessionToken when session-auth feature is disabled");
            return;
        }
        authMethodsConfig.sessionToken = *auth.sessionToken;
    }

    if (auth.xToken)
    {
        if constexpr (!BMCWEB_XTOKEN_AUTH)
        {
            messages::actionNotSupported(
                asyncResp->res,
                "Setting XToken when xtoken-auth feature is disabled");
            return;
        }
        authMethodsConfig.xtoken = *auth.xToken;
    }

    if (auth.tls)
    {
        if constexpr (!BMCWEB_MUTUAL_TLS_AUTH)
        {
            messages::actionNotSupported(
                asyncResp->res,
                "Setting TLS when mutual-tls-auth feature is disabled");
            return;
        }
        authMethodsConfig.tls = *auth.tls;
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

struct LdapPatchParams
{
    std::optional<std::string> authType;
    std::optional<std::vector<std::string>> serviceAddressList;
    std::optional<bool> serviceEnabled;
    std::optional<std::vector<std::string>> baseDNList;
    std::optional<std::string> userNameAttribute;
    std::optional<std::string> groupsAttribute;
    std::optional<std::string> userName;
    std::optional<std::string> password;
    std::optional<
        std::vector<std::variant<nlohmann::json::object_t, std::nullptr_t>>>
        remoteRoleMapData;
};

inline void handleLDAPPatch(LdapPatchParams&& input,
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
        BMCWEB_LOG_ERROR("serverType wasn't AD or LDAP but was {}????",
                         serverType);
        return;
    }

    if (input.authType && *input.authType != "UsernameAndPassword")
    {
        messages::propertyValueNotInList(asyncResp->res, *input.authType,
                                         "AuthenticationType");
        return;
    }

    if (input.serviceAddressList)
    {
        if (input.serviceAddressList->empty())
        {
            messages::propertyValueNotInList(
                asyncResp->res, *input.serviceAddressList, "ServiceAddress");
            return;
        }
    }
    if (input.baseDNList)
    {
        if (input.baseDNList->empty())
        {
            messages::propertyValueNotInList(asyncResp->res, *input.baseDNList,
                                             "BaseDistinguishedNames");
            return;
        }
    }

    // nothing to update, then return
    if (!input.userName && !input.password && !input.serviceAddressList &&
        !input.baseDNList && !input.userNameAttribute &&
        !input.groupsAttribute && !input.serviceEnabled &&
        !input.remoteRoleMapData)
    {
        return;
    }

    // Get the existing resource first then keep modifying
    // whenever any property gets updated.
    getLDAPConfigData(serverType, [asyncResp, input = std::move(input),
                                   dbusObjectPath = std::move(dbusObjectPath)](
                                      bool success,
                                      const LDAPConfigData& confData,
                                      const std::string& serverT) mutable {
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

        if (input.serviceAddressList)
        {
            handleServiceAddressPatch(*input.serviceAddressList, asyncResp,
                                      serverT, dbusObjectPath);
        }
        if (input.userName)
        {
            handleUserNamePatch(*input.userName, asyncResp, serverT,
                                dbusObjectPath);
        }
        if (input.password)
        {
            handlePasswordPatch(*input.password, asyncResp, serverT,
                                dbusObjectPath);
        }

        if (input.baseDNList)
        {
            handleBaseDNPatch(*input.baseDNList, asyncResp, serverT,
                              dbusObjectPath);
        }
        if (input.userNameAttribute)
        {
            handleUserNameAttrPatch(*input.userNameAttribute, asyncResp,
                                    serverT, dbusObjectPath);
        }
        if (input.groupsAttribute)
        {
            handleGroupNameAttrPatch(*input.groupsAttribute, asyncResp, serverT,
                                     dbusObjectPath);
        }
        if (input.serviceEnabled)
        {
            // if user has given the value as true then enable
            // the service. if user has given false then no-op
            // as service is already stopped.
            if (*input.serviceEnabled)
            {
                handleServiceEnablePatch(*input.serviceEnabled, asyncResp,
                                         serverT, dbusObjectPath);
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

        if (input.remoteRoleMapData)
        {
            handleRoleMapPatch(asyncResp, confData.groupRoleList, serverT,
                               *input.remoteRoleMapData);
        }
    });
}

inline void updateUserProperties(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp, const std::string& username,
    const std::optional<std::string>& password,
    const std::optional<bool>& enabled,
    const std::optional<std::string>& roleId, const std::optional<bool>& locked,
    std::optional<std::vector<std::string>> accountTypes, bool userSelf,
    const std::optional<std::vector<std::string>>& mfaBypass,
    const std::shared_ptr<persistent_data::UserSession>& session)
{
    sdbusplus::message::object_path tempObjPath(rootUserDbusPath);
    tempObjPath /= username;
    std::string dbusObjectPath(tempObjPath);

    dbus::utility::checkDbusPathExists(dbusObjectPath, [dbusObjectPath,
                                                        username, password,
                                                        roleId, enabled, locked,
                                                        accountTypes(std::move(
                                                            accountTypes)),
                                                        userSelf, session,
                                                        mfaBypass,
                                                        asyncResp{std::move(
                                                            asyncResp)}](
                                                           int rc) {
        if (rc <= 0)
        {
            messages::resourceNotFound(asyncResp->res, "ManagerAccount",
                                       username);
            return;
        }

        if (password)
        {
            int retval = pamUpdatePassword(username, *password);

            if (retval == PAM_USER_UNKNOWN)
            {
                messages::resourceNotFound(asyncResp->res, "ManagerAccount",
                                           username);
            }
            else if (retval == PAM_AUTHTOK_ERR)
            {
                // If password is invalid
                messages::propertyValueFormatError(asyncResp->res, nullptr,
                                                   "Password");
                BMCWEB_LOG_ERROR("pamUpdatePassword Failed");
            }
            else if (retval != PAM_SUCCESS)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            else
            {
                // Remove existing sessions of the user when password
                // changed
                persistent_data::SessionStore::getInstance()
                    .removeSessionsByUsernameExceptSession(username, session);
                messages::success(asyncResp->res);
            }
        }

        if (mfaBypass)
        {
            std::string mfaBypassDbusVal;
            // Check if the input vector contains just one bypass type:
            // as there are just two defined in the backend:
            // GoogleAuthenticator  and None
            if (mfaBypass->size() > 1)
            {
                messages::propertyNotWritable(asyncResp->res,
                                              "MFABypass/BypassTypes");
                return;
            }
            std::string mfaBypassDbusPrefix =
                "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.";
            if (mfaBypass->empty())
            {
                mfaBypassDbusVal = mfaBypassDbusPrefix + "None";
            }
            else
            {
                std::string mfaBypassVal = mfaBypass->front();
                if (mfaBypassVal == "GoogleAuthenticator" ||
                    mfaBypassVal == "None")
                {
                    mfaBypassDbusVal = mfaBypassDbusPrefix + mfaBypassVal;
                }
                else
                {
                    messages::propertyValueNotInList(
                        asyncResp->res, mfaBypassVal, "MFABypass/BypassTypes");
                    return;
                }
            }
            setDbusProperty(asyncResp, "MFABypass/BypassTypes",
                            "xyz.openbmc_project.User.Manager", dbusObjectPath,
                            "xyz.openbmc_project.User.TOTPAuthenticator",
                            "BypassedProtocol", mfaBypassDbusVal);
        }

        if (enabled)
        {
            setDbusProperty(asyncResp, "Enabled",
                            "xyz.openbmc_project.User.Manager", dbusObjectPath,
                            "xyz.openbmc_project.User.Attributes",
                            "UserEnabled", *enabled);
        }

        if (roleId)
        {
            std::string priv = getPrivilegeFromRoleId(*roleId);
            if (priv.empty())
            {
                messages::propertyValueNotInList(asyncResp->res, true,
                                                 "Locked");
                return;
            }
            setDbusProperty(asyncResp, "RoleId",
                            "xyz.openbmc_project.User.Manager", dbusObjectPath,
                            "xyz.openbmc_project.User.Attributes",
                            "UserPrivilege", priv);
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
            setDbusProperty(asyncResp, "Locked",
                            "xyz.openbmc_project.User.Manager", dbusObjectPath,
                            "xyz.openbmc_project.User.Attributes",
                            "UserLockedForFailedAttempt", *locked);
        }

        if (accountTypes)
        {
            patchAccountTypes(*accountTypes, asyncResp, dbusObjectPath,
                              userSelf);
        }
    });
}

inline void handleAccountServiceHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AccountService/AccountService.json>; rel=describedby");
}

inline void
    getClientCertificates(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const nlohmann::json::json_pointer& keyLocation)
{
    boost::urls::url url(
        "/redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates");
    std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Certs.Certificate"};
    std::string path = "/xyz/openbmc_project/certs/authority/truststore";

    collection_util::getCollectionToKey(asyncResp, url, interfaces, path,
                                        keyLocation);
}

inline void handleAccountServiceClientCertificatesInstanceHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /*id*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Certificate/Certificate.json>; rel=describedby");
}

inline void handleAccountServiceClientCertificatesInstanceGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& id)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    BMCWEB_LOG_DEBUG("ClientCertificate Certificate ID={}", id);
    const boost::urls::url certURL = boost::urls::format(
        "/redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates/{}",
        id);
    std::string objPath =
        sdbusplus::message::object_path(certs::authorityObjectPath) / id;
    getCertificateProperties(
        asyncResp, objPath,
        "xyz.openbmc_project.Certs.Manager.Authority.Truststore", id, certURL,
        "Client Certificate");
}

inline void handleAccountServiceClientCertificatesHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/CertificateCollection/CertificateCollection.json>; rel=describedby");
}

inline void handleAccountServiceClientCertificatesGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    getClientCertificates(asyncResp, "/Members"_json_pointer);
}

using account_service::CertificateMappingAttribute;
using persistent_data::MTLSCommonNameParseMode;
inline CertificateMappingAttribute
    getCertificateMapping(MTLSCommonNameParseMode parse)
{
    switch (parse)
    {
        case MTLSCommonNameParseMode::CommonName:
        {
            return CertificateMappingAttribute::CommonName;
        }
        break;
        case MTLSCommonNameParseMode::Whole:
        {
            return CertificateMappingAttribute::Whole;
        }
        break;
        case MTLSCommonNameParseMode::UserPrincipalName:
        {
            return CertificateMappingAttribute::UserPrincipalName;
        }
        break;

        case MTLSCommonNameParseMode::Meta:
        {
            if constexpr (BMCWEB_META_TLS_COMMON_NAME_PARSING)
            {
                return CertificateMappingAttribute::CommonName;
            }
        }
        break;
        default:
        {
            return CertificateMappingAttribute::Invalid;
        }
        break;
    }
}

inline void getMultiFactorAuthConfiguration(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
        "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.MultiFactorAuthConfiguration", "Enabled",
        [asyncResp](const boost::system::error_code& ec,
                    const std::string& multiFactorAuthEnabledVal) {
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error while fetching MultiFactorAuth property. Error: {}",
                    ec);
                messages::internalError(asyncResp->res);
                return;
            }

            constexpr std::string_view mfaGoogleAuthDbusVal =
                "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator";
            bool googleAuthEnabled =
                (multiFactorAuthEnabledVal == mfaGoogleAuthDbusVal);
            asyncResp->res.jsonValue["MultiFactorAuth"]["GoogleAuthenticator"]
                                    ["Enabled"] = googleAuthEnabled;
        });
}

inline void
    handleAccountServiceGet(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    const persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AccountService/AccountService.json>; rel=describedby");

    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.id"] = "/redfish/v1/AccountService";
    json["@odata.type"] = "#AccountService.v1_15_0.AccountService";
    json["Id"] = "AccountService";
    json["Name"] = "Account Service";
    json["Description"] = "Account Service";
    json["ServiceEnabled"] = true;
    json["MaxPasswordLength"] = 20;
    json["Accounts"]["@odata.id"] = "/redfish/v1/AccountService/Accounts";
    json["Roles"]["@odata.id"] = "/redfish/v1/AccountService/Roles";
    json["HTTPBasicAuth"] = authMethodsConfig.basic
                                ? account_service::BasicAuthState::Enabled
                                : account_service::BasicAuthState::Disabled;

    nlohmann::json::array_t allowed;
    allowed.emplace_back(account_service::BasicAuthState::Enabled);
    allowed.emplace_back(account_service::BasicAuthState::Disabled);
    json["HTTPBasicAuth@AllowableValues"] = std::move(allowed);

    nlohmann::json::object_t clientCertificate;
    clientCertificate["Enabled"] = authMethodsConfig.tls;
    clientCertificate["RespondToUnauthenticatedClients"] =
        !authMethodsConfig.tlsStrict;

    using account_service::CertificateMappingAttribute;

    CertificateMappingAttribute mapping =
        getCertificateMapping(authMethodsConfig.mTLSCommonNameParsingMode);
    if (mapping == CertificateMappingAttribute::Invalid)
    {
        messages::internalError(asyncResp->res);
    }
    else
    {
        clientCertificate["CertificateMappingAttribute"] = mapping;
    }
    nlohmann::json::object_t certificates;
    certificates["@odata.id"] =
        "/redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates";
    certificates["@odata.type"] =
        "#CertificateCollection.CertificateCollection";
    clientCertificate["Certificates"] = std::move(certificates);
    json["MultiFactorAuth"]["ClientCertificate"] = std::move(clientCertificate);

    getClientCertificates(
        asyncResp,
        "/MultiFactorAuth/ClientCertificate/Certificates/Members"_json_pointer);

    json["Oem"]["OpenBMC"]["@odata.type"] =
        "#OpenBMCAccountService.v1_0_0.AccountService";
    json["Oem"]["OpenBMC"]["@odata.id"] =
        "/redfish/v1/AccountService#/Oem/OpenBMC";
    json["Oem"]["OpenBMC"]["AuthMethods"]["BasicAuth"] =
        authMethodsConfig.basic;
    json["Oem"]["OpenBMC"]["AuthMethods"]["SessionToken"] =
        authMethodsConfig.sessionToken;
    json["Oem"]["OpenBMC"]["AuthMethods"]["XToken"] = authMethodsConfig.xtoken;
    json["Oem"]["OpenBMC"]["AuthMethods"]["Cookie"] = authMethodsConfig.cookie;
    json["Oem"]["OpenBMC"]["AuthMethods"]["TLS"] = authMethodsConfig.tls;

    // /redfish/v1/AccountService/LDAP/Certificates is something only
    // ConfigureManager can access then only display when the user has
    // permissions ConfigureManager
    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);

    if (isOperationAllowedWithPrivileges({{"ConfigureManager"}},
                                         effectiveUserPrivileges))
    {
        asyncResp->res.jsonValue["LDAP"]["Certificates"]["@odata.id"] =
            "/redfish/v1/AccountService/LDAP/Certificates";
    }
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
        "/xyz/openbmc_project/user", "xyz.openbmc_project.User.AccountPolicy",
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG("Got {} properties for AccountService",
                             propertiesList.size());

            const uint8_t* minPasswordLength = nullptr;
            const uint32_t* accountUnlockTimeout = nullptr;
            const uint16_t* maxLoginAttemptBeforeLockout = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), propertiesList,
                "MinPasswordLength", minPasswordLength, "AccountUnlockTimeout",
                accountUnlockTimeout, "MaxLoginAttemptBeforeLockout",
                maxLoginAttemptBeforeLockout);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (minPasswordLength != nullptr)
            {
                asyncResp->res.jsonValue["MinPasswordLength"] =
                    *minPasswordLength;
            }

            if (accountUnlockTimeout != nullptr)
            {
                asyncResp->res.jsonValue["AccountLockoutDuration"] =
                    *accountUnlockTimeout;
            }

            if (maxLoginAttemptBeforeLockout != nullptr)
            {
                asyncResp->res.jsonValue["AccountLockoutThreshold"] =
                    *maxLoginAttemptBeforeLockout;
            }
        });

    // Get MFA Config
    getMultiFactorAuthConfiguration(asyncResp);

    auto callback = [asyncResp](bool success, const LDAPConfigData& confData,
                                const std::string& ldapType) {
        if (!success)
        {
            return;
        }
        parseLDAPConfigData(asyncResp->res.jsonValue, confData, ldapType);
    };

    getLDAPConfigData("LDAP", callback);
    getLDAPConfigData("ActiveDirectory", callback);
}

inline void handleCertificateMappingAttributePatch(
    crow::Response& res, const std::string& certMapAttribute)
{
    MTLSCommonNameParseMode parseMode =
        persistent_data::getMTLSCommonNameParseMode(certMapAttribute);
    if (parseMode == MTLSCommonNameParseMode::Invalid)
    {
        messages::propertyValueNotInList(res, "CertificateMappingAttribute",
                                         certMapAttribute);
        return;
    }

    persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();
    authMethodsConfig.mTLSCommonNameParsingMode = parseMode;
}

inline void handleRespondToUnauthenticatedClientsPatch(
    App& app, const crow::Request& req, crow::Response& res,
    bool respondToUnauthenticatedClients)
{
    if (req.session != nullptr)
    {
        // Sanity check.  If the user isn't currently authenticated with mutual
        // TLS, they very likely are about to permanently lock themselves out.
        // Make sure they're using mutual TLS before allowing locking.
        if (req.session->sessionType != persistent_data::SessionType::MutualTLS)
        {
            messages::propertyValueExternalConflict(
                res,
                "MultiFactorAuth/ClientCertificate/RespondToUnauthenticatedClients",
                respondToUnauthenticatedClients);
            return;
        }
    }

    persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    // Change the settings
    authMethodsConfig.tlsStrict = !respondToUnauthenticatedClients;

    // Write settings to disk
    persistent_data::getConfig().writeData();

    // Trigger a reload, to apply the new settings to new connections
    app.loadCertificate();
}

inline void handleAccountServicePatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<uint32_t> unlockTimeout;
    std::optional<uint16_t> lockoutThreshold;
    std::optional<uint8_t> minPasswordLength;
    std::optional<uint16_t> maxPasswordLength;
    LdapPatchParams ldapObject;
    std::optional<std::string> certificateMappingAttribute;
    std::optional<bool> respondToUnauthenticatedClients;
    LdapPatchParams activeDirectoryObject;
    AuthMethods auth;
    std::optional<std::string> httpBasicAuth;
    std::optional<bool> googleAuthenticatorEnabled;

    if (!json_util::readJsonPatch( //
            req, asyncResp->res, //
            "AccountLockoutDuration", unlockTimeout, //
            "AccountLockoutThreshold", lockoutThreshold, //
            "ActiveDirectory/Authentication/AuthenticationType",
            activeDirectoryObject.authType, //
            "ActiveDirectory/Authentication/Password",
            activeDirectoryObject.password, //
            "ActiveDirectory/Authentication/Username",
            activeDirectoryObject.userName, //
            "ActiveDirectory/LDAPService/SearchSettings/BaseDistinguishedNames",
            activeDirectoryObject.baseDNList, //
            "ActiveDirectory/LDAPService/SearchSettings/GroupsAttribute",
            activeDirectoryObject.groupsAttribute, //
            "ActiveDirectory/LDAPService/SearchSettings/UsernameAttribute",
            activeDirectoryObject.userNameAttribute, //
            "ActiveDirectory/RemoteRoleMapping",
            activeDirectoryObject.remoteRoleMapData, //
            "ActiveDirectory/ServiceAddresses",
            activeDirectoryObject.serviceAddressList, //
            "ActiveDirectory/ServiceEnabled",
            activeDirectoryObject.serviceEnabled, //
            "HTTPBasicAuth", httpBasicAuth, //
            "LDAP/Authentication/AuthenticationType", ldapObject.authType, //
            "LDAP/Authentication/Password", ldapObject.password, //
            "LDAP/Authentication/Username", ldapObject.userName, //
            "LDAP/LDAPService/SearchSettings/BaseDistinguishedNames",
            ldapObject.baseDNList, //
            "LDAP/LDAPService/SearchSettings/GroupsAttribute",
            ldapObject.groupsAttribute, //
            "LDAP/LDAPService/SearchSettings/UsernameAttribute",
            ldapObject.userNameAttribute, //
            "LDAP/RemoteRoleMapping", ldapObject.remoteRoleMapData, //
            "LDAP/ServiceAddresses", ldapObject.serviceAddressList, //
            "LDAP/ServiceEnabled", ldapObject.serviceEnabled, //
            "MaxPasswordLength", maxPasswordLength, //
            "MinPasswordLength", minPasswordLength, //
            "MultiFactorAuth/ClientCertificate/CertificateMappingAttribute",
            certificateMappingAttribute, //
            "MultiFactorAuth/ClientCertificate/RespondToUnauthenticatedClients",
            respondToUnauthenticatedClients, //
            "MultiFactorAuth/GoogleAuthenticator/Enabled",
            googleAuthenticatorEnabled, //
            "Oem/OpenBMC/AuthMethods/BasicAuth", auth.basicAuth, //
            "Oem/OpenBMC/AuthMethods/Cookie", auth.cookie, //
            "Oem/OpenBMC/AuthMethods/SessionToken", auth.sessionToken, //
            "Oem/OpenBMC/AuthMethods/TLS", auth.tls, //
            "Oem/OpenBMC/AuthMethods/XToken", auth.xToken //
            ))
    {
        return;
    }

    if (httpBasicAuth)
    {
        if (*httpBasicAuth == "Enabled")
        {
            auth.basicAuth = true;
        }
        else if (*httpBasicAuth == "Disabled")
        {
            auth.basicAuth = false;
        }
        else
        {
            messages::propertyValueNotInList(asyncResp->res, "HttpBasicAuth",
                                             *httpBasicAuth);
        }
    }

    if (respondToUnauthenticatedClients)
    {
        handleRespondToUnauthenticatedClientsPatch(
            app, req, asyncResp->res, *respondToUnauthenticatedClients);
    }

    if (certificateMappingAttribute)
    {
        handleCertificateMappingAttributePatch(asyncResp->res,
                                               *certificateMappingAttribute);
    }

    if (minPasswordLength)
    {
        setDbusProperty(
            asyncResp, "MinPasswordLength", "xyz.openbmc_project.User.Manager",
            sdbusplus::message::object_path("/xyz/openbmc_project/user"),
            "xyz.openbmc_project.User.AccountPolicy", "MinPasswordLength",
            *minPasswordLength);
    }

    if (maxPasswordLength)
    {
        messages::propertyNotWritable(asyncResp->res, "MaxPasswordLength");
    }

    handleLDAPPatch(std::move(activeDirectoryObject), asyncResp,
                    "ActiveDirectory");
    handleLDAPPatch(std::move(ldapObject), asyncResp, "LDAP");

    handleAuthMethodsPatch(asyncResp, auth);

    if (unlockTimeout)
    {
        setDbusProperty(
            asyncResp, "AccountLockoutDuration",
            "xyz.openbmc_project.User.Manager",
            sdbusplus::message::object_path("/xyz/openbmc_project/user"),
            "xyz.openbmc_project.User.AccountPolicy", "AccountUnlockTimeout",
            *unlockTimeout);
    }
    if (lockoutThreshold)
    {
        setDbusProperty(
            asyncResp, "AccountLockoutThreshold",
            "xyz.openbmc_project.User.Manager",
            sdbusplus::message::object_path("/xyz/openbmc_project/user"),
            "xyz.openbmc_project.User.AccountPolicy",
            "MaxLoginAttemptBeforeLockout", *lockoutThreshold);
    }
    if (googleAuthenticatorEnabled)
    {
        std::string_view mfaType =
            "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.None";
        if (*googleAuthenticatorEnabled)
        {
            mfaType =
                "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator";
        }

        setDbusProperty(
            asyncResp, "MultiFactorAuth/GoogleAuthenticator/Enabled",
            "xyz.openbmc_project.User.Manager",
            sdbusplus::message::object_path("/xyz/openbmc_project/user"),
            "xyz.openbmc_project.User.MultiFactorAuthConfiguration", "Enabled",
            std::string(mfaType));
    }
}

inline void handleAccountCollectionHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ManagerAccountCollection.json>; rel=describedby");
}

inline void handleAccountCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ManagerAccountCollection.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/AccountService/Accounts";
    asyncResp->res.jsonValue["@odata.type"] = "#ManagerAccountCollection."
                                              "ManagerAccountCollection";
    asyncResp->res.jsonValue["Name"] = "Accounts Collection";
    asyncResp->res.jsonValue["Description"] = "BMC User Accounts";

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);

    std::string thisUser;
    if (req.session)
    {
        thisUser = req.session->username;
    }
    sdbusplus::message::object_path path("/xyz/openbmc_project/user");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.User.Manager", path,
        [asyncResp, thisUser, effectiveUserPrivileges](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& users) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            bool userCanSeeAllAccounts =
                effectiveUserPrivileges.isSupersetOf({"ConfigureUsers"});

            bool userCanSeeSelf =
                effectiveUserPrivileges.isSupersetOf({"ConfigureSelf"});

            nlohmann::json& memberArray = asyncResp->res.jsonValue["Members"];
            memberArray = nlohmann::json::array();

            for (const auto& userpath : users)
            {
                std::string user = userpath.first.filename();
                if (user.empty())
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR("Invalid firmware ID");

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
                    nlohmann::json::object_t member;
                    member["@odata.id"] = boost::urls::format(
                        "/redfish/v1/AccountService/Accounts/{}", user);
                    memberArray.emplace_back(std::move(member));
                }
            }
            asyncResp->res.jsonValue["Members@odata.count"] =
                memberArray.size();
        });
}

inline void processAfterCreateUser(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& username, const std::string& password,
    const boost::system::error_code& ec, sdbusplus::message_t& m)
{
    if (ec)
    {
        userErrorMessageHandler(m.get_error(), asyncResp, username, "");
        return;
    }

    if (pamUpdatePassword(username, password) != PAM_SUCCESS)
    {
        // At this point we have a user that's been
        // created, but the password set
        // failed.Something is wrong, so delete the user
        // that we've already created
        sdbusplus::message::object_path tempObjPath(rootUserDbusPath);
        tempObjPath /= username;
        const std::string userPath(tempObjPath);

        crow::connections::systemBus->async_method_call(
            [asyncResp, password](const boost::system::error_code& ec3) {
                if (ec3)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                // If password is invalid
                messages::propertyValueFormatError(asyncResp->res, nullptr,
                                                   "Password");
            },
            "xyz.openbmc_project.User.Manager", userPath,
            "xyz.openbmc_project.Object.Delete", "Delete");

        BMCWEB_LOG_ERROR("pamUpdatePassword Failed");
        return;
    }

    messages::created(asyncResp->res);
    asyncResp->res.addHeader("Location",
                             "/redfish/v1/AccountService/Accounts/" + username);
}

inline void processAfterGetAllGroups(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& username, const std::string& password,
    const std::string& roleId, bool enabled,
    std::optional<std::vector<std::string>> accountTypes,
    const std::vector<std::string>& allGroupsList)
{
    std::vector<std::string> userGroups;
    std::vector<std::string> accountTypeUserGroups;

    // If user specified account types then convert them to unix user groups
    if (accountTypes)
    {
        if (!getUserGroupFromAccountType(asyncResp->res, *accountTypes,
                                         accountTypeUserGroups))
        {
            // Problem in mapping Account Types to User Groups, Error already
            // logged.
            return;
        }
    }

    for (const auto& grp : allGroupsList)
    {
        // If user specified the account type then only accept groups which are
        // in the account types group list.
        if (!accountTypeUserGroups.empty())
        {
            bool found = false;
            for (const auto& grp1 : accountTypeUserGroups)
            {
                if (grp == grp1)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                continue;
            }
        }

        // Console access is provided to the user who is a member of
        // hostconsole group and has a administrator role. So, set
        // hostconsole group only for the administrator.
        if ((grp == "hostconsole") && (roleId != "priv-admin"))
        {
            if (!accountTypeUserGroups.empty())
            {
                BMCWEB_LOG_ERROR(
                    "Only administrator can get HostConsole access");
                asyncResp->res.result(boost::beast::http::status::bad_request);
                return;
            }
            continue;
        }
        userGroups.emplace_back(grp);
    }

    // Make sure user specified groups are valid. This is internal error because
    // it some inconsistencies between user manager and bmcweb.
    if (!accountTypeUserGroups.empty() &&
        accountTypeUserGroups.size() != userGroups.size())
    {
        messages::internalError(asyncResp->res);
        return;
    }
    crow::connections::systemBus->async_method_call(
        [asyncResp, username, password](const boost::system::error_code& ec2,
                                        sdbusplus::message_t& m) {
            processAfterCreateUser(asyncResp, username, password, ec2, m);
        },
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.Manager", "CreateUser", username, userGroups,
        roleId, enabled);
}

inline void handleAccountCollectionPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string username;
    std::string password;
    std::optional<std::string> roleIdJson;
    std::optional<bool> enabledJson;
    std::optional<std::vector<std::string>> accountTypes;
    if (!json_util::readJsonPatch( //
            req, asyncResp->res, //
            "AccountTypes", accountTypes, //
            "Enabled", enabledJson, //
            "Password", password, //
            "RoleId", roleIdJson, //
            "UserName", username //
            ))
    {
        return;
    }

    std::string roleId = roleIdJson.value_or("User");
    std::string priv = getPrivilegeFromRoleId(roleId);
    if (priv.empty())
    {
        messages::propertyValueNotInList(asyncResp->res, roleId, "RoleId");
        return;
    }
    roleId = priv;

    bool enabled = enabledJson.value_or(true);

    // Reading AllGroups property
    sdbusplus::asio::getProperty<std::vector<std::string>>(
        *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
        "/xyz/openbmc_project/user", "xyz.openbmc_project.User.Manager",
        "AllGroups",
        [asyncResp, username, password{std::move(password)}, roleId, enabled,
         accountTypes](const boost::system::error_code& ec,
                       const std::vector<std::string>& allGroupsList) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("D-Bus response error {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            if (allGroupsList.empty())
            {
                messages::internalError(asyncResp->res);
                return;
            }

            processAfterGetAllGroups(asyncResp, username, password, roleId,
                                     enabled, accountTypes, allGroupsList);
        });
}

inline void
    handleAccountHead(App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& /*accountName*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ManagerAccount/ManagerAccount.json>; rel=describedby");
}

inline void
    handleAccountGet(App& app, const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& accountName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ManagerAccount/ManagerAccount.json>; rel=describedby");

    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        // If authentication is disabled, there are no user accounts
        messages::resourceNotFound(asyncResp->res, "ManagerAccount",
                                   accountName);
        return;
    }

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    if (req.session->username != accountName)
    {
        // At this point we've determined that the user is trying to
        // modify a user that isn't them.  We need to verify that they
        // have permissions to modify other users, so re-run the auth
        // check with the same permissions, minus ConfigureSelf.
        Privileges effectiveUserPrivileges =
            redfish::getUserPrivileges(*req.session);
        Privileges requiredPermissionsToChangeNonSelf = {"ConfigureUsers",
                                                         "ConfigureManager"};
        if (!effectiveUserPrivileges.isSupersetOf(
                requiredPermissionsToChangeNonSelf))
        {
            BMCWEB_LOG_DEBUG("GET Account denied access");
            messages::insufficientPrivilege(asyncResp->res);
            return;
        }
    }

    sdbusplus::message::object_path path("/xyz/openbmc_project/user");
    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.User.Manager", path,
        [asyncResp,
         accountName](const boost::system::error_code& ec,
                      const dbus::utility::ManagedObjectType& users) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            const auto userIt = std::ranges::find_if(
                users,
                [accountName](
                    const std::pair<sdbusplus::message::object_path,
                                    dbus::utility::DBusInterfacesMap>& user) {
                    return accountName == user.first.filename();
                });

            if (userIt == users.end())
            {
                messages::resourceNotFound(asyncResp->res, "ManagerAccount",
                                           accountName);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#ManagerAccount.v1_13_0.ManagerAccount";
            asyncResp->res.jsonValue["Name"] = "User Account";
            asyncResp->res.jsonValue["Description"] = "User Account";
            asyncResp->res.jsonValue["Password"] = nullptr;
            asyncResp->res.jsonValue["StrictAccountTypes"] = true;

            for (const auto& interface : userIt->second)
            {
                if (interface.first == "xyz.openbmc_project.User.Attributes")
                {
                    for (const auto& property : interface.second)
                    {
                        if (property.first == "UserEnabled")
                        {
                            const bool* userEnabled =
                                std::get_if<bool>(&property.second);
                            if (userEnabled == nullptr)
                            {
                                BMCWEB_LOG_ERROR("UserEnabled wasn't a bool");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res.jsonValue["Enabled"] = *userEnabled;
                        }
                        else if (property.first == "UserLockedForFailedAttempt")
                        {
                            const bool* userLocked =
                                std::get_if<bool>(&property.second);
                            if (userLocked == nullptr)
                            {
                                BMCWEB_LOG_ERROR("UserLockedForF"
                                                 "ailedAttempt "
                                                 "wasn't a bool");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res.jsonValue["Locked"] = *userLocked;
                            nlohmann::json::array_t allowed;
                            // can only unlock accounts
                            allowed.emplace_back("false");
                            asyncResp->res
                                .jsonValue["Locked@Redfish.AllowableValues"] =
                                std::move(allowed);
                        }
                        else if (property.first == "UserPrivilege")
                        {
                            const std::string* userPrivPtr =
                                std::get_if<std::string>(&property.second);
                            if (userPrivPtr == nullptr)
                            {
                                BMCWEB_LOG_ERROR("UserPrivilege wasn't a "
                                                 "string");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            std::string role =
                                getRoleIdFromPrivilege(*userPrivPtr);
                            if (role.empty())
                            {
                                BMCWEB_LOG_ERROR("Invalid user role");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res.jsonValue["RoleId"] = role;

                            nlohmann::json& roleEntry =
                                asyncResp->res.jsonValue["Links"]["Role"];
                            roleEntry["@odata.id"] = boost::urls::format(
                                "/redfish/v1/AccountService/Roles/{}", role);
                        }
                        else if (property.first == "UserPasswordExpired")
                        {
                            const bool* userPasswordExpired =
                                std::get_if<bool>(&property.second);
                            if (userPasswordExpired == nullptr)
                            {
                                BMCWEB_LOG_ERROR(
                                    "UserPasswordExpired wasn't a bool");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res.jsonValue["PasswordChangeRequired"] =
                                *userPasswordExpired;
                        }
                        else if (property.first == "UserGroups")
                        {
                            const std::vector<std::string>* userGroups =
                                std::get_if<std::vector<std::string>>(
                                    &property.second);
                            if (userGroups == nullptr)
                            {
                                BMCWEB_LOG_ERROR(
                                    "userGroups wasn't a string vector");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            if (!translateUserGroup(*userGroups,
                                                    asyncResp->res))
                            {
                                BMCWEB_LOG_ERROR("userGroups mapping failed");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                        }
                    }
                }
                if (interface.first ==
                    "xyz.openbmc_project.User.TOTPAuthenticator")
                {
                    for (const auto& property : interface.second)
                    {
                        if (property.first == "BypassedProtocol")
                        {
                            const std::string* bypassedProtocol =
                                std::get_if<std::string>(&property.second);

                            if (bypassedProtocol == nullptr)
                            {
                                BMCWEB_LOG_ERROR(
                                    "BypassedProtocol Value fetch faile");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            nlohmann::json& mfaBypassArray =
                                asyncResp->res
                                    .jsonValue["MFABypass"]["BypassTypes"];
                            mfaBypassArray = nlohmann::json::array();

                            constexpr std::string_view mfaGoogleAuthDbusVal =
                                "xyz.openbmc_project.User.MultiFactorAuthConfiguration.Type.GoogleAuthenticator";
                            if (*bypassedProtocol == mfaGoogleAuthDbusVal)
                            {
                                mfaBypassArray.push_back("GoogleAuthenticator");
                            }
                        }
                        if (property.first == "SecretKeyIsValid")
                        {
                            const bool* secretKeySet =
                                std::get_if<bool>(&property.second);
                            if (secretKeySet == nullptr)
                            {
                                BMCWEB_LOG_ERROR(
                                    "SecretKeyIsValid value fetch failed");
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res.jsonValue["SecretKeySet"] =
                                *secretKeySet;
                        }
                    }
                }
            }

            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/AccountService/Accounts/{}", accountName);
            asyncResp->res.jsonValue["Id"] = accountName;
            asyncResp->res.jsonValue["UserName"] = accountName;
            asyncResp->res
                .jsonValue["Actions"]["#ManagerAccount.GenerateSecretKey"]
                          ["target"] = std::format(
                "/redfish/v1/AccountService/Accounts/{}/Actions/ManagerAccount.GenerateSecretKey",
                accountName);
        });
}

inline void
    handleAccountDelete(App& app, const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& username)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        // If authentication is disabled, there are no user accounts
        messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
        return;
    }
    sdbusplus::message::object_path tempObjPath(rootUserDbusPath);
    tempObjPath /= username;
    const std::string userPath(tempObjPath);

    crow::connections::systemBus->async_method_call(
        [asyncResp, username](const boost::system::error_code& ec) {
            if (ec)
            {
                messages::resourceNotFound(asyncResp->res, "ManagerAccount",
                                           username);
                return;
            }

            messages::accountRemoved(asyncResp->res);
        },
        "xyz.openbmc_project.User.Manager", userPath,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

inline void
    handleAccountPatch(App& app, const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& username)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        // If authentication is disabled, there are no user accounts
        messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
        return;
    }
    std::optional<std::string> newUserName;
    std::optional<std::string> password;
    std::optional<bool> enabled;
    std::optional<std::string> roleId;
    std::optional<bool> locked;
    std::optional<std::vector<std::string>> accountTypes;
    std::optional<std::vector<std::string>> mfaBypass;

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    bool userSelf = (username == req.session->username);

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);
    Privileges configureUsers = {"ConfigureUsers"};
    bool userHasConfigureUsers =
        effectiveUserPrivileges.isSupersetOf(configureUsers);
    if (userHasConfigureUsers)
    {
        // Users with ConfigureUsers can modify for all users
        if (!json_util::readJsonPatch( //
                req, asyncResp->res, //
                "AccountTypes", accountTypes, //
                "Enabled", enabled, //
                "Locked", locked, //
                "MFABypass/BypassTypes", mfaBypass, //
                "Password", password, //
                "RoleId", roleId, //
                "UserName", newUserName //
                ))
        {
            return;
        }
    }
    else
    {
        // ConfigureSelf accounts can only modify their own account
        if (!userSelf)
        {
            messages::insufficientPrivilege(asyncResp->res);
            return;
        }

        // ConfigureSelf accounts can only modify their password
        if (!json_util::readJsonPatch(req, asyncResp->res, "Password",
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
        updateUserProperties(asyncResp, username, password, enabled, roleId,
                             locked, accountTypes, userSelf, mfaBypass,
                             req.session);
        return;
    }
    crow::connections::systemBus->async_method_call(
        [asyncResp, username, password(std::move(password)),
         roleId(std::move(roleId)), enabled, newUser{std::string(*newUserName)},
         locked, userSelf, req, accountTypes(std::move(accountTypes)),
         mfaBypass](const boost::system::error_code& ec,
                    sdbusplus::message_t& m) {
            if (ec)
            {
                userErrorMessageHandler(m.get_error(), asyncResp, newUser,
                                        username);
                return;
            }

            updateUserProperties(asyncResp, newUser, password, enabled, roleId,
                                 locked, accountTypes, userSelf, mfaBypass,
                                 req.session);
        },
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.Manager", "RenameUser", username,
        *newUserName);
}

static void checkAndCreateSecretKey(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& username, const std::string& userPath)
{
    sdbusplus::message::object_path userObjPath(
        "/xyz/openbmc_project/user/" + username);
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
        std::string(userObjPath), "xyz.openbmc_project.User.TOTPAuthenticator",
        "SecretKeyIsValid",
        [asyncResp, username, userPath](const boost::system::error_code& ec,
                                        const bool& isSecretKeySet) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("D-Bus response error: {}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }

            if (isSecretKeySet)
            {
                BMCWEB_LOG_WARNING("Secret key is already setup for the user");
                messages::actionNotSupported(asyncResp->res,
                                             "GenerateSecretKey");
                return;
            }

            // If secret key is not set, then proceed to create it
            crow::connections::systemBus->async_method_call(
                [asyncResp, username](const boost::system::error_code& ec1,
                                      const std::string& secretKey) {
                    if (ec1)
                    {
                        BMCWEB_LOG_ERROR("D-Bus response error: {}",
                                         ec1.value());
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue["@odata.type"] =
                        "#ManagerAccount.v1_13_0.ManagerAccount";
                    asyncResp->res.jsonValue["Name"] = "User Account";
                    asyncResp->res.jsonValue["Description"] = "User Account";
                    asyncResp->res.jsonValue["SecretKey"] = secretKey;
                },
                "xyz.openbmc_project.User.Manager", userPath,
                "xyz.openbmc_project.User.TOTPAuthenticator",
                "CreateSecretKey");
        });
}

inline void
    handleGenerateSecretKey(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& username)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_INSECURE_DISABLE_AUTH)
    {
        // If authentication is disabled, there are no user accounts
        messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
        return;
    }

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    bool userSelf = (username == req.session->username);

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);
    Privileges configureUsers = {"ConfigureUsers"};
    bool userHasConfigureUsers =
        effectiveUserPrivileges.isSupersetOf(configureUsers);

    if (!userHasConfigureUsers && !userSelf)
    {
        BMCWEB_LOG_WARNING("Insufficient Privilege");
        messages::insufficientPrivilege(asyncResp->res);
        return;
    }
    sdbusplus::message::object_path tempObjPath(rootUserDbusPath);
    tempObjPath /= username;
    const std::string userPath(tempObjPath);

    checkAndCreateSecretKey(asyncResp, username, userPath);
}

inline void requestAccountServiceRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/")
        .privileges(redfish::privileges::headAccountService)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleAccountServiceHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/")
        .privileges(redfish::privileges::getAccountService)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleAccountServiceGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/")
        .privileges(redfish::privileges::patchAccountService)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleAccountServicePatch, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates/")
        .privileges(redfish::privileges::headCertificateCollection)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleAccountServiceClientCertificatesHead, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates/")
        .privileges(redfish::privileges::getCertificateCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleAccountServiceClientCertificatesGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates/<str>/")
        .privileges(redfish::privileges::headCertificate)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleAccountServiceClientCertificatesInstanceHead, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/MultiFactorAuth/ClientCertificate/Certificates/<str>/")
        .privileges(redfish::privileges::getCertificate)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleAccountServiceClientCertificatesInstanceGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
        .privileges(redfish::privileges::headManagerAccountCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleAccountCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
        .privileges(redfish::privileges::getManagerAccountCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleAccountCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/")
        .privileges(redfish::privileges::postManagerAccountCollection)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleAccountCollectionPost, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        .privileges(redfish::privileges::headManagerAccount)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleAccountHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        .privileges(redfish::privileges::getManagerAccount)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleAccountGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        // TODO this privilege should be using the generated endpoints, but
        // because of the special handling of ConfigureSelf, it's not able to
        // yet
        .privileges({{"ConfigureUsers"}, {"ConfigureSelf"}})
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleAccountPatch, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/AccountService/Accounts/<str>/")
        .privileges(redfish::privileges::deleteManagerAccount)
        .methods(boost::beast::http::verb::delete_)(
            std::bind_front(handleAccountDelete, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/AccountService/Accounts/<str>/Actions/ManagerAccount.GenerateSecretKey")
        // Only the user is authorized to configure or set up MFA for their own
        // account.
        .privileges(redfish::privileges::postManagerAccount)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleGenerateSecretKey, std::ref(app)));
}

} // namespace redfish
