// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "dbus_singleton.hpp"
#include "sessions.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace bmcweb
{

using DbusUserPropVariant =
    std::variant<std::vector<std::string>, std::string, bool>;
using DbusUserObjPath = sdbusplus::message::object_path;
using DbusUserObjProperties =
    std::vector<std::pair<std::string, DbusUserPropVariant>>;
using DbusUserObjValue = std::map<std::string, DbusUserObjProperties>;

// Object Manager signals
static constexpr const char* intfAddedSignal = "InterfacesAdded";
static constexpr const char* intfRemovedSignal = "InterfacesRemoved";
// User manager signal memebers
static constexpr const char* userRenamedSignal = "UserRenamed";
static constexpr const char* userPropertyChanged = "PropertiesChanged";
/* Default name of first bootStrap account */
static constexpr const char* firstUserName = "bootstrap0";

static constexpr const char* dBusPropertiesInterface =
    "org.freedesktop.DBus.Properties";
static constexpr const char* propertiesChangedSignal = "PropertiesChanged";
// Object Manager related
static constexpr const char* dBusObjManager =
    "org.freedesktop.DBus.ObjectManager";
// User manager related
static constexpr const char* userMgrObjBasePath = "/xyz/openbmc_project/user";
static constexpr const char* userObjBasePath = "/xyz/openbmc_project/user";
static constexpr const char* userMgrInterface =
    "xyz.openbmc_project.User.Manager";
static constexpr const char* usersInterface =
    "xyz.openbmc_project.User.Attributes";
static constexpr const char* usersBootStrapAccount = "BootStrapAccount";

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::vector<std::string> listBootStrapUser{};

inline bool isBootStrapAccount(const std::string& userName)
{
    return std::find(listBootStrapUser.begin(), listBootStrapUser.end(),
                     userName) != listBootStrapUser.end();
}

inline void userUpdatedSignalHandler(sdbusplus::message_t& msg)
{
    std::string signal = msg.get_member();
    std::string userName;
    std::string newUserName;

    if (signal == intfAddedSignal)
    {
        DbusUserObjPath objPath;
        DbusUserObjValue objValue;
        msg.read(objPath, objValue);
        userName = objPath.filename();

        if (userName == firstUserName)
        {
            if (std::find(listBootStrapUser.begin(), listBootStrapUser.end(),
                          userName) == listBootStrapUser.end())
            {
                listBootStrapUser.emplace_back(userName);
            }
        }
    }
    else if (signal == intfRemovedSignal)
    {
        DbusUserObjPath objPath;
        std::vector<std::string> interfaces;
        msg.read(objPath, interfaces);
        userName = objPath.filename();

        listBootStrapUser.erase(std::find(listBootStrapUser.begin(),
                                          listBootStrapUser.end(), userName));

        persistent_data::SessionStore::getInstance().removeSessionsByUsername(
            userName);
    }
    else if (signal == userRenamedSignal)
    {
        msg.read(userName, newUserName);

        if (std::find(listBootStrapUser.begin(), listBootStrapUser.end(),
                      userName) != listBootStrapUser.end())
        {
            listBootStrapUser.erase(std::find(
                listBootStrapUser.begin(), listBootStrapUser.end(), userName));
            if (std::find(listBootStrapUser.begin(), listBootStrapUser.end(),
                          newUserName) == listBootStrapUser.end())
            {
                listBootStrapUser.emplace_back(newUserName);
            }
        }
    }
    else if (signal == userPropertyChanged)
    {
        DbusUserObjProperties props;
        std::string iface;
        sdbusplus::message::object_path objPath(msg.get_path());
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
                if (std::find(listBootStrapUser.begin(),
                              listBootStrapUser.end(), userName) ==
                    listBootStrapUser.end())
                {
                    listBootStrapUser.emplace_back(userName);
                }
            }
            else
            {
                listBootStrapUser.erase(
                    std::find(listBootStrapUser.begin(),
                              listBootStrapUser.end(), userName));
            }
        }
    }
}

inline void registerUserSignal()
{
    static sdbusplus::bus::match_t userUpdatedSignal(
        *crow::connections::systemBus,
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::interface(dBusObjManager) +
            sdbusplus::bus::match::rules::path(userMgrObjBasePath),
        [&](sdbusplus::message_t& msg) { userUpdatedSignalHandler(msg); });
    static sdbusplus::bus::match_t userMgrRenamedSignal(
        *crow::connections::systemBus,
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::interface(userMgrInterface) +
            sdbusplus::bus::match::rules::path(userMgrObjBasePath),
        [&](sdbusplus::message_t& msg) { userUpdatedSignalHandler(msg); });
    static sdbusplus::bus::match_t userPropertiesSignal(
        *crow::connections::systemBus,
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::path_namespace(userObjBasePath) +
            sdbusplus::bus::match::rules::interface(dBusPropertiesInterface) +
            sdbusplus::bus::match::rules::member(propertiesChangedSignal) +
            sdbusplus::bus::match::rules::argN(0, usersInterface),
        [&](sdbusplus::message_t& msg) { userUpdatedSignalHandler(msg); });
}

} // namespace bmcweb
