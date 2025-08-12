// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "logging.hpp"
#include "sessions.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <string>
#include <variant>
#include <vector>

namespace bmcweb
{

using DbusUserPropVariant =
    std::variant<std::vector<std::string>, std::string, bool>;
using DbusUserObjPath = sdbusplus::object_path;
using DbusUserObjProperties =
    std::vector<std::pair<std::string, DbusUserPropVariant>>;

// Object Manager signals
static constexpr const char* intfAddedSignal = "InterfacesAdded";
static constexpr const char* intfRemovedSignal = "InterfacesRemoved";
// User manager signal memebers
static constexpr const char* userRenamedSignal = "UserRenamed";
static constexpr const char* userPropertyChanged = "PropertiesChanged";

static constexpr const char* dBusPropertiesInterface =
    "org.freedesktop.DBus.Properties";
static constexpr const char* propertiesChangedSignal = "PropertiesChanged";
// Object Manager related
static constexpr const char* dBusObjManager =
    "org.freedesktop.DBus.ObjectManager";
// User manager related
static constexpr const char* userMgrObjBasePath = "/xyz/openbmc_project/user";
static constexpr const char* userObjBasePath = "/xyz/openbmc_project/user";
static constexpr const char* userMgrService =
    "xyz.openbmc_project.User.Manager";
static constexpr const char* userMgrInterface =
    "xyz.openbmc_project.User.Manager";
static constexpr const char* usersInterface =
    "xyz.openbmc_project.User.Attributes";
static constexpr const char* usersBootStrapAccount = "BootStrapAccount";

inline std::vector<std::string>& getBootStrapUserList()
{
    static std::vector<std::string> listBootStrapUser;
    return listBootStrapUser;
}

inline bool isBootStrapAccount(const std::string& userName)
{
    std::vector<std::string>& listBootStrapUser = getBootStrapUserList();
    return std::ranges::find(listBootStrapUser, userName) !=
           listBootStrapUser.end();
}

inline void addBootStrapUser(const std::string& userName)
{
    std::vector<std::string>& listBootStrapUser = getBootStrapUserList();
    if (std::ranges::find(listBootStrapUser, userName) ==
        listBootStrapUser.end())
    {
        listBootStrapUser.emplace_back(userName);
    }
}

// Bootstrap accounts are identified by the BootStrapAccount D-Bus property,
// not by name.
inline bool isBootStrapAccountSet(
    const dbus::utility::DBusInterfacesMap& interfaces)
{
    const auto it = std::ranges::find_if(interfaces, [](const auto& iface) {
        return iface.first == usersInterface;
    });
    if (it == interfaces.end())
    {
        return false;
    }
    const bool* bootStrapAccount = nullptr;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), it->second,
        usersBootStrapAccount, bootStrapAccount);
    return success && bootStrapAccount != nullptr && *bootStrapAccount;
}

inline void userUpdatedSignalHandler(sdbusplus::message_t& msg)
{
    std::vector<std::string>& listBootStrapUser = getBootStrapUserList();
    std::string signal = msg.get_member();
    std::string userName;
    std::string newUserName;

    if (signal == intfAddedSignal)
    {
        DbusUserObjPath objPath;
        dbus::utility::DBusInterfacesMap interfaces;
        msg.read(objPath, interfaces);
        userName = objPath.filename();

        if (isBootStrapAccountSet(interfaces))
        {
            addBootStrapUser(userName);
        }
    }
    else if (signal == intfRemovedSignal)
    {
        DbusUserObjPath objPath;
        std::vector<std::string> interfaces;
        msg.read(objPath, interfaces);
        userName = objPath.filename();

        auto it = std::ranges::find(listBootStrapUser, userName);
        if (it != listBootStrapUser.end())
        {
            listBootStrapUser.erase(it);
        }

        persistent_data::SessionStore::getInstance().removeSessionsByUsername(
            userName);
    }
    else if (signal == userRenamedSignal)
    {
        msg.read(userName, newUserName);

        if (std::ranges::find(listBootStrapUser, userName) !=
            listBootStrapUser.end())
        {
            listBootStrapUser.erase(
                std::ranges::find(listBootStrapUser, userName));
            if (std::ranges::find(listBootStrapUser, newUserName) ==
                listBootStrapUser.end())
            {
                listBootStrapUser.emplace_back(newUserName);
            }
        }
    }
    else if (signal == userPropertyChanged)
    {
        DbusUserObjProperties props;
        std::string iface;
        sdbusplus::object_path objPath(msg.get_path());
        userName = objPath.filename();

        msg.read(iface, props);
        if (iface != usersInterface)
        {
            return;
        }
        for (const auto& prop : props)
        {
            if ((prop.first != usersBootStrapAccount))
            {
                continue;
            }
            if (std::get<bool>(prop.second))
            {
                if (std::ranges::find(listBootStrapUser, userName) ==
                    listBootStrapUser.end())
                {
                    listBootStrapUser.emplace_back(userName);
                }
            }
            else
            {
                auto it = std::ranges::find(listBootStrapUser, userName);
                if (it != listBootStrapUser.end())
                {
                    listBootStrapUser.erase(it);
                }
            }
        }
    }
}

inline void afterGetUsersForBootStrapCleanup(
    const boost::system::error_code& ec,
    const dbus::utility::ManagedObjectType& objects)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("Failed to get users: {}", ec.message());
        return;
    }
    for (const auto& [path, interfaces] : objects)
    {
        if (!isBootStrapAccountSet(interfaces))
        {
            continue;
        }
        BMCWEB_LOG_INFO("Deleting bootstrap account {}", path.filename());
        dbus::utility::async_method_call(
            [userName = path.filename()](const boost::system::error_code& ec2) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR(
                        "Failed to delete bootstrap account {}: {}", userName,
                        ec2.message());
                }
            },
            userMgrService, path.str, "xyz.openbmc_project.Object.Delete",
            "Delete");
    }
}

// DSP0270: delete all bootstrap accounts on any reset of the service.
inline void deleteBootStrapUsers()
{
    dbus::utility::getManagedObjects(userMgrService,
                                     sdbusplus::object_path(userObjBasePath),
                                     afterGetUsersForBootStrapCleanup);
}

inline void registerUserSignal()
{
    static sdbusplus::bus::match_t userUpdatedSignal(
        *crow::connections::systemBus,
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::interface(dBusObjManager) +
            sdbusplus::bus::match::rules::path(userMgrObjBasePath),
        userUpdatedSignalHandler);
    static sdbusplus::bus::match_t userMgrRenamedSignal(
        *crow::connections::systemBus,
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::interface(userMgrInterface) +
            sdbusplus::bus::match::rules::path(userMgrObjBasePath),
        userUpdatedSignalHandler);
    static sdbusplus::bus::match_t userPropertiesSignal(
        *crow::connections::systemBus,
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::path_namespace(userObjBasePath) +
            sdbusplus::bus::match::rules::interface(dBusPropertiesInterface) +
            sdbusplus::bus::match::rules::member(propertiesChangedSignal) +
            sdbusplus::bus::match::rules::argN(0, usersInterface),
        userUpdatedSignalHandler);
    deleteBootStrapUsers();
}

// Drop a user's Redfish sessions whenever User.Attributes.UserEnabled
// transitions to false. Covers Redfish PATCH, IPMI `user disable`, and any
// other writer of the property.
inline void onUserPropertiesChanged(sdbusplus::message_t& msg)
{
    std::string interface;
    dbus::utility::DBusPropertiesMap propertiesMap;
    try
    {
        msg.read(interface, propertiesMap);
    }
    catch (const sdbusplus::exception_t& e)
    {
        BMCWEB_LOG_ERROR("Failed to read PropertiesChanged signal: {}",
                         e.what());
        return;
    }

    if (interface != "xyz.openbmc_project.User.Attributes")
    {
        return;
    }

    const bool* userEnabled = nullptr;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        redfish::dbus_utils::UnpackErrorPrinter(), propertiesMap, "UserEnabled",
        userEnabled);
    if (!success || userEnabled == nullptr || *userEnabled)
    {
        // Property wasn't in this change, or user is being (re-)enabled.
        return;
    }

    sdbusplus::object_path path(msg.get_path());
    std::string username = path.filename();
    if (username.empty())
    {
        return;
    }

    BMCWEB_LOG_INFO("User {} disabled; clearing active sessions", username);
    persistent_data::SessionStore::getInstance().removeSessionsByUsername(
        username);
}

inline void registerUserPropertiesChangedSignal()
{
    std::string matchStr = sdbusplus::match_rules::propertiesChangedNamespace(
        "/xyz/openbmc_project/user", "xyz.openbmc_project.User.Attributes");

    static sdbusplus::match userPropertiesChangedMatch(
        *crow::connections::systemBus, matchStr, onUserPropertiesChanged);
}
} // namespace bmcweb
