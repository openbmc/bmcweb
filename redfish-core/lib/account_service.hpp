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

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/account_service.hpp"
#include "openbmc_dbus_rest.hpp"
#include "persistent_data.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "roles.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
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
    if (role == "priv-oemibmserviceagent")
    {
        return "OemIBMServiceAgent";
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
    if (role == "OemIBMServiceAgent")
    {
        return "priv-oemibmserviceagent";
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
inline bool
    getUserGroupFromAccountType(crow::Response& res,
                                const std::vector<std::string>& accountTypes,
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

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
        dbusObjectPath, "xyz.openbmc_project.User.Attributes", "UserGroups",
        updatedUserGroups, [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("D-Bus responses error: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        messages::success(asyncResp->res);
    });
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
    std::string service = (ldapType == "LDAP") ? "LDAPService"
                                               : "ActiveDirectoryService";

    nlohmann::json& ldap = jsonResponse[ldapType];

    ldap["ServiceEnabled"] = confData.serviceEnabled;
    ldap["ServiceAddresses"] = nlohmann::json::array({confData.uri});
    ldap["Authentication"]["AuthenticationType"] =
        account_service::AuthenticationTypes::UsernameAndPassword;
    ldap["Authentication"]["Username"] = confData.bindDN;
    ldap["Authentication"]["Password"] = nullptr;

    ldap["LDAPService"]["SearchSettings"]["BaseDistinguishedNames"] =
        nlohmann::json::array({confData.baseDN});
    ldap["LDAPService"]["SearchSettings"]["UsernameAttribute"] =
        confData.userNameAttribute;
    ldap["LDAPService"]["SearchSettings"]["GroupsAttribute"] =
        confData.groupAttribute;

    nlohmann::json& roleMapArray = ldap["RemoteRoleMapping"];
    roleMapArray = nlohmann::json::array();
    for (const auto& obj : confData.groupRoleList)
    {
        BMCWEB_LOG_DEBUG("Pushing the data groupName={}", obj.second.groupName);

        nlohmann::json::object_t remoteGroup;
        remoteGroup["RemoteGroup"] = obj.second.groupName;
        remoteGroup["LocalRole"] = getRoleIdFromPrivilege(obj.second.privilege);
        roleMapArray.emplace_back(std::move(remoteGroup));
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
                     index](const boost::system::error_code& ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR("DBUS response error: {}", ec);
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    asyncResp->res.jsonValue[serverType]["RemoteRoleMapping"]
                                            [index] = nullptr;
                },
                    ldapDbusService, roleMapObjData[index].first,
                    "xyz.openbmc_project.Object.Delete", "Delete");
            }
            else
            {
                BMCWEB_LOG_ERROR("Can't delete the object");
                messages::propertyValueTypeError(asyncResp->res, thisJson,
                                                 "RemoteRoleMapping/" +
                                                     std::to_string(index));
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

            // Do not allow mapping to a Restricted LocalRole
            if (localRole && redfish::isRestrictedRole(*localRole))
            {
                messages::restrictedRole(asyncResp->res, *localRole);
                return;
            }

            // Update existing RoleMapping Object
            if (index < roleMapObjData.size())
            {
                BMCWEB_LOG_DEBUG("Update Role Map Object");
                // If "RemoteGroup" info is provided
                if (remoteGroup)
                {
                    sdbusplus::asio::setProperty(
                        *crow::connections::systemBus, ldapDbusService,
                        roleMapObjData[index].first,
                        "xyz.openbmc_project.User.PrivilegeMapperEntry",
                        "GroupName", *remoteGroup,
                        [asyncResp, roleMapObjData, serverType, index,
                         remoteGroup](const boost::system::error_code& ec,
                                      const sdbusplus::message_t& msg) {
                        if (ec)
                        {
                            const sd_bus_error* dbusError = msg.get_error();
                            if ((dbusError != nullptr) &&
                                (dbusError->name ==
                                 std::string_view(
                                     "xyz.openbmc_project.Common.Error.InvalidArgument")))
                            {
                                BMCWEB_LOG_WARNING("DBUS response error: {}",
                                                   ec);
                                messages::propertyValueIncorrect(asyncResp->res,
                                                                 "RemoteGroup",
                                                                 *remoteGroup);
                                return;
                            }
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res
                            .jsonValue[serverType]["RemoteRoleMapping"][index]
                                      ["RemoteGroup"] = *remoteGroup;
                    });
                }

                // If "LocalRole" info is provided
                if (localRole)
                {
                    sdbusplus::asio::setProperty(
                        *crow::connections::systemBus, ldapDbusService,
                        roleMapObjData[index].first,
                        "xyz.openbmc_project.User.PrivilegeMapperEntry",
                        "Privilege", *localRole,
                        [asyncResp, roleMapObjData, serverType, index,
                         localRole](const boost::system::error_code& ec,
                                    const sdbusplus::message_t& msg) {
                        if (ec)
                        {
                            const sd_bus_error* dbusError = msg.get_error();
                            if ((dbusError != nullptr) &&
                                (dbusError->name ==
                                 std::string_view(
                                     "xyz.openbmc_project.Common.Error.InvalidArgument")))
                            {
                                BMCWEB_LOG_WARNING("DBUS response error: {}",
                                                   ec);
                                messages::propertyValueIncorrect(
                                    asyncResp->res, "LocalRole", *localRole);
                                return;
                            }
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        asyncResp->res
                            .jsonValue[serverType]["RemoteRoleMapping"][index]
                                      ["LocalRole"] = *localRole;
                    });
                }
            }
            // Create a new RoleMapping Object.
            else
            {
                BMCWEB_LOG_DEBUG(
                    "setRoleMappingProperties: Creating new Object");
                std::string pathString = "RemoteRoleMapping/" +
                                         std::to_string(index);

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
inline void getLDAPConfigData(const std::string& ldapType,
                              CallbackFunc&& callback)
{
    constexpr std::array<std::string_view, 2> interfaces = {
        ldapEnableInterface, ldapConfigInterface};

    dbus::utility::getDbusObject(
        ldapConfigObjectName, interfaces,
        [callback, ldapType](const boost::system::error_code& ec,
                             const dbus::utility::MapperGetObject& resp) {
        if (ec || resp.empty())
        {
            BMCWEB_LOG_WARNING(
                "DBUS response error during getting of service name: {}", ec);
            LDAPConfigData empty{};
            callback(false, empty, ldapType);
            return;
        }
        std::string service = resp.begin()->first;
        sdbusplus::message::object_path path(ldapRootObject);
        dbus::utility::getManagedObjects(
            service, path,
            [callback,
             ldapType](const boost::system::error_code& ec2,
                       const dbus::utility::ManagedObjectType& ldapObjects) {
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
                BMCWEB_LOG_ERROR("Can't get the DbusType for the given type={}",
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
                if (object.first.str.find(searchString) == std::string::npos)
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
                                std::get_if<std::string>(&property.second);
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
                            else if (property.first == "LDAPSearchScope")
                            {
                                confData.searchScope = *strValue;
                            }
                            else if (property.first == "GroupNameAttribute")
                            {
                                confData.groupAttribute = *strValue;
                            }
                            else if (property.first == "UserNameAttribute")
                            {
                                confData.userNameAttribute = *strValue;
                            }
                            else if (property.first == "LDAPType")
                            {
                                confData.serverType = *strValue;
                            }
                        }
                    }
                    else if (interface.first ==
                             "xyz.openbmc_project.User.PrivilegeMapperEntry")
                    {
                        LDAPRoleMapData roleMapData{};
                        for (const auto& property : interface.second)
                        {
                            const std::string* strValue =
                                std::get_if<std::string>(&property.second);

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

                        confData.groupRoleList.emplace_back(object.first.str,
                                                            roleMapData);
                    }
                }
            }
            callback(true, confData, ldapType);
        });
    });
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
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, ldapDbusService, ldapConfigObject,
        ldapConfigInterface, "LDAPServerURI", serviceAddressList.front(),
        [asyncResp, ldapServerElementName, serviceAddressList](
            const boost::system::error_code& ec, sdbusplus::message_t& msg) {
        if (ec)
        {
            const sd_bus_error* dbusError = msg.get_error();
            if ((dbusError != nullptr) &&
                (dbusError->name ==
                 std::string_view(
                     "xyz.openbmc_project.Common.Error.InvalidArgument")))
            {
                BMCWEB_LOG_WARNING(
                    "Error Occurred in updating the service address");
                messages::propertyValueIncorrect(asyncResp->res,
                                                 "ServiceAddresses",
                                                 serviceAddressList.front());
                return;
            }
            messages::internalError(asyncResp->res);
            return;
        }
        std::vector<std::string> modifiedserviceAddressList = {
            serviceAddressList.front()};
        asyncResp->res.jsonValue[ldapServerElementName]["ServiceAddresses"] =
            modifiedserviceAddressList;
        if ((serviceAddressList).size() > 1)
        {
            messages::propertyValueModified(asyncResp->res, "ServiceAddresses",
                                            serviceAddressList.front());
        }
        BMCWEB_LOG_DEBUG("Updated the service address");
    });
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
    sdbusplus::asio::setProperty(*crow::connections::systemBus, ldapDbusService,
                                 ldapConfigObject, ldapConfigInterface,
                                 "LDAPBindDN", username,
                                 [asyncResp, username, ldapServerElementName](
                                     const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("Error occurred in updating the username");
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue[ldapServerElementName]["Authentication"]
                                ["Username"] = username;
        BMCWEB_LOG_DEBUG("Updated the username");
    });
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
    sdbusplus::asio::setProperty(*crow::connections::systemBus, ldapDbusService,
                                 ldapConfigObject, ldapConfigInterface,
                                 "LDAPBindDNPassword", password,
                                 [asyncResp, password, ldapServerElementName](
                                     const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("Error occurred in updating the password");
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue[ldapServerElementName]["Authentication"]
                                ["Password"] = "";
        BMCWEB_LOG_DEBUG("Updated the password");
    });
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
    sdbusplus::asio::setProperty(*crow::connections::systemBus, ldapDbusService,
                                 ldapConfigObject, ldapConfigInterface,
                                 "LDAPBaseDN", baseDNList.front(),
                                 [asyncResp, baseDNList, ldapServerElementName](
                                     const boost::system::error_code& ec,
                                     const sdbusplus::message_t& msg) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("Error Occurred in Updating the base DN");
            const sd_bus_error* dbusError = msg.get_error();
            if ((dbusError != nullptr) &&
                (dbusError->name ==
                 std::string_view(
                     "xyz.openbmc_project.Common.Error.InvalidArgument")))
            {
                messages::propertyValueIncorrect(asyncResp->res,
                                                 "BaseDistinguishedNames",
                                                 baseDNList.front());
                return;
            }
            messages::internalError(asyncResp->res);
            return;
        }
        auto& serverTypeJson = asyncResp->res.jsonValue[ldapServerElementName];
        auto& searchSettingsJson =
            serverTypeJson["LDAPService"]["SearchSettings"];
        std::vector<std::string> modifiedBaseDNList = {baseDNList.front()};
        searchSettingsJson["BaseDistinguishedNames"] = modifiedBaseDNList;
        if (baseDNList.size() > 1)
        {
            messages::propertyValueModified(
                asyncResp->res, "BaseDistinguishedNames", baseDNList.front());
        }
        BMCWEB_LOG_DEBUG("Updated the base DN");
    });
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
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, ldapDbusService, ldapConfigObject,
        ldapConfigInterface, "UserNameAttribute", userNameAttribute,
        [asyncResp, userNameAttribute,
         ldapServerElementName](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("Error Occurred in Updating the "
                             "username attribute");
            messages::internalError(asyncResp->res);
            return;
        }
        auto& serverTypeJson = asyncResp->res.jsonValue[ldapServerElementName];
        auto& searchSettingsJson =
            serverTypeJson["LDAPService"]["SearchSettings"];
        searchSettingsJson["UsernameAttribute"] = userNameAttribute;
        BMCWEB_LOG_DEBUG("Updated the user name attr.");
    });
}

inline void getAcfProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::tuple<std::vector<uint8_t>, bool, std::string>& messageFDbus)
{
    asyncResp->res.jsonValue["Oem"]["IBM"]["@odata.type"] =
        "#OemManagerAccount.v1_0_0.IBM";
    asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]["@odata.type"] =
        "#OemManagerAccount.v1_0_0.ACF";
    // Get messages from call to InstallACF and add values to json
    std::vector<uint8_t> acfFile = std::get<0>(messageFDbus);
    std::string decodeACFFile(acfFile.begin(), acfFile.end());
    std::string encodedACFFile = crow::utility::base64encode(decodeACFFile);

    bool acfInstalled = std::get<1>(messageFDbus);
    std::string expirationDate = std::get<2>(messageFDbus);

    asyncResp->res
        .jsonValue["Oem"]["IBM"]["ACF"]["WarningLongDatedExpiration"] = nullptr;
    asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]["ACFFile"] = nullptr;
    asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]["ExpirationDate"] = nullptr;

    if (acfInstalled)
    {
        asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]["ExpirationDate"] =
            expirationDate;

        asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]["ACFFile"] =
            encodedACFFile;

        std::time_t result = std::time(nullptr);

        // YYYY-MM-DD format
        // Parse expirationDate to get difference between now and expiration
        std::string expirationDateCpy = expirationDate;
        std::string delimiter = "-";
        std::vector<int> parseTime;

        char* endPtr = nullptr;
        size_t pos = 0;
        std::string token;
        // expirationDate should be exactly 10 characters
        if (expirationDateCpy.length() != 10)
        {
            BMCWEB_LOG_ERROR("expirationDate format invalid");
            asyncResp->res = {};
            messages::internalError(asyncResp->res);
            return;
        }
        while ((pos = expirationDateCpy.find(delimiter)) != std::string::npos)
        {
            token = expirationDateCpy.substr(0, pos);
            parseTime.push_back(
                static_cast<int>(std::strtol(token.c_str(), &endPtr, 10)));

            if (*endPtr != '\0')
            {
                BMCWEB_LOG_ERROR("expirationDate format enum");
                asyncResp->res = {};
                messages::internalError(asyncResp->res);
                return;
            }
            expirationDateCpy.erase(0, pos + delimiter.length());
        }
        parseTime.push_back(static_cast<int>(
            std::strtol(expirationDateCpy.c_str(), &endPtr, 10)));

        // Expect 3 sections. YYYY MM DD
        if (*endPtr != '\0' && parseTime.size() != 3)
        {
            BMCWEB_LOG_ERROR("expirationDate format invalid");
            messages::internalError(asyncResp->res);
            return;
        }

        std::tm tm{}; // zero initialise
        tm.tm_year = parseTime.at(0) - 1900;
        tm.tm_mon = parseTime.at(1) - 1;
        tm.tm_mday = parseTime.at(2);

        std::time_t t = std::mktime(&tm);
        u_int diffTime = static_cast<u_int>(std::difftime(t, result));
        // BMC date is displayed if exp date > 30 days
        // 30 days = 30 * 24 * 60 * 60 seconds
        if (diffTime > 2592000)
        {
            asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]
                                    ["WarningLongDatedExpiration"] = true;
        }
        else
        {
            asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]
                                    ["WarningLongDatedExpiration"] = false;
        }
    }
    asyncResp->res.jsonValue["Oem"]["IBM"]["ACF"]["ACFInstalled"] =
        acfInstalled;
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
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, ldapDbusService, ldapConfigObject,
        ldapConfigInterface, "GroupNameAttribute", groupsAttribute,
        [asyncResp, groupsAttribute,
         ldapServerElementName](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("Error Occurred in Updating the "
                             "groupname attribute");
            messages::internalError(asyncResp->res);
            return;
        }
        auto& serverTypeJson = asyncResp->res.jsonValue[ldapServerElementName];
        auto& searchSettingsJson =
            serverTypeJson["LDAPService"]["SearchSettings"];
        searchSettingsJson["GroupsAttribute"] = groupsAttribute;
        BMCWEB_LOG_DEBUG("Updated the groupname attr");
    });
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
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, ldapDbusService, ldapConfigObject,
        ldapEnableInterface, "Enabled", serviceEnabled,
        [asyncResp, serviceEnabled,
         ldapServerElementName](const boost::system::error_code& ec) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("Error Occurred in Updating the service enable");
            messages::internalError(asyncResp->res);
            return;
        }
        asyncResp->res.jsonValue[ldapServerElementName]["ServiceEnabled"] =
            serviceEnabled;
        BMCWEB_LOG_DEBUG("Updated Service enable = {}", serviceEnabled);
    });
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
        BMCWEB_LOG_ERROR("Cannot read values from AuthMethod tag");
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
        if (serviceAddressList->empty())
        {
            messages::propertyValueNotInList(
                asyncResp->res, *serviceAddressList, "ServiceAddress");
            return;
        }
    }
    if (baseDNList)
    {
        if (baseDNList->empty())
        {
            messages::propertyValueNotInList(asyncResp->res, *baseDNList,
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
    getLDAPConfigData(
        serverType,
        [asyncResp, userName, password, baseDNList, userNameAttribute,
         groupsAttribute, serviceAddressList, serviceEnabled, dbusObjectPath,
         remoteRoleMapData](bool success, const LDAPConfigData& confData,
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

inline void updateUserProperties(
    std::shared_ptr<bmcweb::AsyncResp> asyncResp, const std::string& username,
    const std::optional<std::string>& password,
    const std::optional<bool>& enabled,
    const std::optional<std::string>& roleId, const std::optional<bool>& locked,
    std::optional<std::vector<std::string>> accountTypes, bool userSelf)
{
    sdbusplus::message::object_path tempObjPath(rootUserDbusPath);
    tempObjPath /= username;
    std::string dbusObjectPath(tempObjPath);

    dbus::utility::checkDbusPathExists(
        dbusObjectPath, [dbusObjectPath, username, password, roleId, enabled,
                         locked, accountTypes(std::move(accountTypes)),
                         userSelf, asyncResp{std::move(asyncResp)}](int rc) {
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
                messages::success(asyncResp->res);
            }
        }

        if (enabled)
        {
            sdbusplus::asio::setProperty(
                *crow::connections::systemBus,
                "xyz.openbmc_project.User.Manager", dbusObjectPath,
                "xyz.openbmc_project.User.Attributes", "UserEnabled", *enabled,
                [asyncResp](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("D-Bus responses error: {}", ec);
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            });
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

            sdbusplus::asio::setProperty(
                *crow::connections::systemBus,
                "xyz.openbmc_project.User.Manager", dbusObjectPath,
                "xyz.openbmc_project.User.Attributes", "UserPrivilege", priv,
                [asyncResp](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("D-Bus responses error: {}", ec);
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            });
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

            sdbusplus::asio::setProperty(
                *crow::connections::systemBus,
                "xyz.openbmc_project.User.Manager", dbusObjectPath,
                "xyz.openbmc_project.User.Attributes",
                "UserLockedForFailedAttempt", *locked,
                [asyncResp](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("D-Bus responses error: {}", ec);
                    messages::internalError(asyncResp->res);
                    return;
                }
                messages::success(asyncResp->res);
            });
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

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/AccountService/AccountService.json>; rel=describedby");

    const persistent_data::AuthConfigMethods& authMethodsConfig =
        persistent_data::SessionStore::getInstance().getAuthMethodsConfig();

    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.id"] = "/redfish/v1/AccountService";
    json["@odata.type"] = "#AccountService."
                          "v1_10_0.AccountService";
    json["Id"] = "AccountService";
    json["Name"] = "Account Service";
    json["Description"] = "Account Service";
    json["ServiceEnabled"] = true;
    json["MaxPasswordLength"] = 20;
    json["Accounts"]["@odata.id"] = "/redfish/v1/AccountService/Accounts";
    json["Roles"]["@odata.id"] = "/redfish/v1/AccountService/Roles";
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

        BMCWEB_LOG_DEBUG("Got {}properties for AccountService",
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
            asyncResp->res.jsonValue["MinPasswordLength"] = *minPasswordLength;
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
    std::optional<nlohmann::json> ldapObject;
    std::optional<nlohmann::json> activeDirectoryObject;
    std::optional<nlohmann::json> oemObject;

    if (!json_util::readJsonPatch(
            req, asyncResp->res, "AccountLockoutDuration", unlockTimeout,
            "AccountLockoutThreshold", lockoutThreshold, "MaxPasswordLength",
            maxPasswordLength, "MinPasswordLength", minPasswordLength, "LDAP",
            ldapObject, "ActiveDirectory", activeDirectoryObject, "Oem",
            oemObject))
    {
        return;
    }

    if (minPasswordLength)
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.AccountPolicy", "MinPasswordLength",
            *minPasswordLength,
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        });
    }

    if (maxPasswordLength)
    {
        messages::propertyNotWritable(asyncResp->res, "MaxPasswordLength");
    }

    if (ldapObject)
    {
        handleLDAPPatch(*ldapObject, asyncResp, "LDAP");
    }

    if (std::optional<nlohmann::json> oemOpenBMCObject;
        oemObject && json_util::readJson(*oemObject, asyncResp->res, "OpenBMC",
                                         oemOpenBMCObject))
    {
        if (std::optional<nlohmann::json> authMethodsObject;
            oemOpenBMCObject &&
            json_util::readJson(*oemOpenBMCObject, asyncResp->res,
                                "AuthMethods", authMethodsObject))
        {
            if (authMethodsObject)
            {
                handleAuthMethodsPatch(*authMethodsObject, asyncResp);
            }
        }
    }

    if (activeDirectoryObject)
    {
        handleLDAPPatch(*activeDirectoryObject, asyncResp, "ActiveDirectory");
    }

    if (unlockTimeout)
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.AccountPolicy", "AccountUnlockTimeout",
            *unlockTimeout, [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        });
    }
    if (lockoutThreshold)
    {
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, "xyz.openbmc_project.User.Manager",
            "/xyz/openbmc_project/user",
            "xyz.openbmc_project.User.AccountPolicy",
            "MaxLoginAttemptBeforeLockout", *lockoutThreshold,
            [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        });
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
            if (userCanSeeAllAccounts || (thisUser == user && userCanSeeSelf))
            {
                nlohmann::json::object_t member;
                member["@odata.id"] = boost::urls::format(
                    "/redfish/v1/AccountService/Accounts/{}", user);
                memberArray.emplace_back(std::move(member));
            }
        }
        asyncResp->res.jsonValue["Members@odata.count"] = memberArray.size();
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
    if (!json_util::readJsonPatch(req, asyncResp->res, "UserName", username,
                                  "Password", password, "RoleId", roleIdJson,
                                  "Enabled", enabledJson, "AccountTypes",
                                  accountTypes))
    {
        return;
    }

    std::string roleId = roleIdJson.value_or("User");
    std::string priv = getPrivilegeFromRoleId(roleId);

    // Don't allow new accounts to have a Restricted Role.
    if (redfish::isRestrictedRole(roleId))
    {
        messages::restrictedRole(asyncResp->res, roleId);
        return;
    }

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
            BMCWEB_LOG_DEBUG("ERROR with async_method_call");
            messages::internalError(asyncResp->res);
            return;
        }

        if (allGroupsList.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }

        processAfterGetAllGroups(asyncResp, username, password, roleId, enabled,
                                 accountTypes, allGroupsList);
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

#ifdef BMCWEB_INSECURE_DISABLE_AUTHENTICATION
    // If authentication is disabled, there are no user accounts
    messages::resourceNotFound(asyncResp->res, "ManagerAccount", accountName);
    return;
#endif // BMCWEB_INSECURE_DISABLE_AUTHENTICATION

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
            "#ManagerAccount.v1_7_0.ManagerAccount";
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
                        asyncResp->res
                            .jsonValue["Locked@Redfish.AllowableValues"] = {
                            "false"}; // can only unlock accounts
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
                        std::string role = getRoleIdFromPrivilege(*userPrivPtr);
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
                        if (!translateUserGroup(*userGroups, asyncResp->res))
                        {
                            BMCWEB_LOG_ERROR("userGroups mapping failed");
                            messages::internalError(asyncResp->res);
                            return;
                        }
                    }
                }
            }
        }

        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/AccountService/Accounts/{}", accountName);
        asyncResp->res.jsonValue["Id"] = accountName;
        asyncResp->res.jsonValue["UserName"] = accountName;

        if (accountName == "service")
        {
            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec2,
                            const std::tuple<std::vector<uint8_t>, bool,
                                             std::string>& messageFDbus) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR("DBUS response error:{}", ec2);
                    messages::internalError(asyncResp->res);
                    return;
                }
                getAcfProperties(asyncResp, messageFDbus);
            },
                "xyz.openbmc_project.Certs.ACF.Manager",
                "/xyz/openbmc_project/certs/ACF",
                "xyz.openbmc_project.Certs.ACF", "GetACFInfo");
        }
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

#ifdef BMCWEB_INSECURE_DISABLE_AUTHENTICATION
    // If authentication is disabled, there are no user accounts
    messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
    return;

#endif // BMCWEB_INSECURE_DISABLE_AUTHENTICATION
    sdbusplus::message::object_path tempObjPath(rootUserDbusPath);
    tempObjPath /= username;
    const std::string userPath(tempObjPath);

    // Don't DELETE accounts which have a Restricted Role (the service account).
    // Implementation note: Ideally this would get the user's RoleId
    // but that would take an additional D-Bus operation.
    if (username == "service")
    {
        BMCWEB_LOG_ERROR("Attempt to DELETE user who has a Restricted Role");
        messages::restrictedRole(asyncResp->res, "OemIBMServiceAgent");
        return;
    }

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
#ifdef BMCWEB_INSECURE_DISABLE_AUTHENTICATION
    // If authentication is disabled, there are no user accounts
    messages::resourceNotFound(asyncResp->res, "ManagerAccount", username);
    return;

#endif // BMCWEB_INSECURE_DISABLE_AUTHENTICATION
    std::optional<std::string> newUserName;
    std::optional<std::string> password;
    std::optional<bool> enabled;
    std::optional<std::string> roleId;
    std::optional<bool> locked;
    std::optional<std::vector<std::string>> accountTypes;
    bool userSelf = (username == req.session->username);
    std::optional<nlohmann::json> oem;

    if (req.session == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(*req.session);
    Privileges configureUsers = {"ConfigureUsers"};
    bool userHasConfigureUsers =
        effectiveUserPrivileges.isSupersetOf(configureUsers);
    if (userHasConfigureUsers)
    {
        // Users with ConfigureUsers can modify for all users

        if (!json_util::readJsonPatch(req, asyncResp->res, "UserName",
                                      newUserName, "Password", password,
                                      "RoleId", roleId, "Enabled", enabled,
                                      "Locked", locked, "Oem", oem))
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

    // For accounts which have a Restricted Role, restrict which properties
    // can be patched.  Allow only Locked, Enabled, and Oem.
    // Do not even allow the service user to change these properties.
    // Implementation note: Ideally this would get the user's RoleId
    // but that would take an additional D-Bus operation.
    if ((username == "service") && (newUserName || password || roleId))
    {
        BMCWEB_LOG_ERROR("Attempt to PATCH user who has a Restricted Role");
        messages::restrictedRole(asyncResp->res, "OemIBMServiceAgent");
        return;
    }

    // Don't allow PATCHing an account to have a Restricted role.
    if (roleId && redfish::isRestrictedRole(*roleId))
    {
        BMCWEB_LOG_ERROR("Attempt to PATCH user to have a Restricted Role");
        messages::restrictedRole(asyncResp->res, *roleId);
        return;
    }

    if (oem)
    {
        std::optional<nlohmann::json> ibm;
        if (!redfish::json_util::readJson(*oem, asyncResp->res, "IBM", ibm))
        {
            BMCWEB_LOG_ERROR("Illegal Property ");
            messages::propertyMissing(asyncResp->res, "IBM");
            return;
        }
        if (ibm)
        {
            std::optional<nlohmann::json> acf;
            if (!redfish::json_util::readJson(*ibm, asyncResp->res, "ACF", acf))
            {
                BMCWEB_LOG_ERROR("Illegal Property ");
                messages::propertyMissing(asyncResp->res, "ACF");
                return;
            }
            if (acf && (username == "service"))
            {
                std::vector<uint8_t> decodedAcf;
                std::optional<std::string> acfFile;
                if (acf.value().contains("ACFFile") &&
                    acf.value()["ACFFile"] == nullptr)
                {
                    acfFile = "";
                }
                else
                {
                    if (!redfish::json_util::readJson(*acf, asyncResp->res,
                                                      "ACFFile", acfFile))
                    {
                        BMCWEB_LOG_ERROR("Illegal Property ");
                        messages::propertyMissing(asyncResp->res, "ACFFile");
                        return;
                    }

                    std::string sDecodedAcf;
                    if (!crow::utility::base64Decode(*acfFile, sDecodedAcf))
                    {
                        BMCWEB_LOG_ERROR("base64 decode failure ");
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    try
                    {
                        std::copy(sDecodedAcf.begin(), sDecodedAcf.end(),
                                  std::back_inserter(decodedAcf));
                    }
                    catch (const std::exception& e)
                    {
                        BMCWEB_LOG_ERROR("{}", e.what());
                        messages::internalError(asyncResp->res);
                        return;
                    }
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                sdbusplus::message::message& m,
                                const std::tuple<std::vector<uint8_t>, bool,
                                                 std::string>& messageFDbus) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR("DBUS response error:{}", ec);
                        if (strcmp(m.get_error()->name,
                                   "xyz.openbmc_project.Certs."
                                   "Error.InvalidCertificate") == 0)
                        {
                            redfish::messages::invalidUpload(
                                asyncResp->res,
                                "/redfish/v1/AccountService/"
                                "Accounts/service",
                                "Invalid Certificate");
                        }
                        else
                        {
                            messages::internalError(asyncResp->res);
                        }
                        return;
                    }
                    getAcfProperties(asyncResp, messageFDbus);
                },
                    "xyz.openbmc_project.Certs.ACF.Manager",
                    "/xyz/openbmc_project/certs/ACF",
                    "xyz.openbmc_project.Certs.ACF", "InstallACF", decodedAcf);
            }
            else if (acf && (username != "service"))
            {
                messages::resourceAtUriUnauthorized(
                    asyncResp->res, req.url(),
                    "ACF properties access not allowed by non service "
                    "user");
            }
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
                             locked, accountTypes, userSelf);
        return;
    }
    crow::connections::systemBus->async_method_call(
        [asyncResp, username, password(std::move(password)),
         roleId(std::move(roleId)), enabled, newUser{std::string(*newUserName)},
         locked, userSelf, accountTypes(std::move(accountTypes))](
            const boost::system::error_code& ec, sdbusplus::message_t& m) {
        if (ec)
        {
            userErrorMessageHandler(m.get_error(), asyncResp, newUser,
                                    username);
            return;
        }

        updateUserProperties(asyncResp, newUser, password, enabled, roleId,
                             locked, accountTypes, userSelf);
    },
        "xyz.openbmc_project.User.Manager", "/xyz/openbmc_project/user",
        "xyz.openbmc_project.User.Manager", "RenameUser", username,
        *newUserName);
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
}

} // namespace redfish
