#pragma once

#include "dbus_utility.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace crow
{
struct UserFields
{
    std::optional<std::string> userRole;
    std::optional<bool> remote;
    std::optional<bool> passwordExpired;
    std::optional<std::vector<std::string>> userGroups;
};

struct UserRoleMap
{
  public:
    static UserRoleMap& getInstance()
    {
        static UserRoleMap userRoleMap;
        return userRoleMap;
    }

    UserFields getUserRole(std::string_view name)
    {
        auto it = roleMap.find(name);
        if (it == roleMap.end())
        {
            BMCWEB_LOG_ERROR("User name {} is not found in the UserRoleMap.",
                             name);
            return {};
        }
        return it->second;
    }
    UserRoleMap(const UserRoleMap&) = delete;
    UserRoleMap& operator=(const UserRoleMap&) = delete;
    UserRoleMap(UserRoleMap&&) = delete;
    UserRoleMap& operator=(UserRoleMap&&) = delete;
    ~UserRoleMap() = default;

  private:
    static UserFields extractUserRole(
        const dbus::utility::DBusInteracesMap& interfacesProperties)
    {
        UserFields fields;
        for (const auto& interface : interfacesProperties)
        {
            for (const auto& property : interface.second)
            {
                if (property.first == "UserPrivilege")
                {
                    const std::string* role =
                        std::get_if<std::string>(&property.second);
                    if (role != nullptr)
                    {
                        fields.userRole = *role;
                    }
                }
                else if (property.first == "UserGroups")
                {
                    const std::vector<std::string>* groups =
                        std::get_if<std::vector<std::string>>(&property.second);
                    if (groups != nullptr)
                    {
                        fields.userGroups = *groups;
                    }
                }
                else if (property.first == "UserPasswordExpired")
                {
                    const bool* expired = std::get_if<bool>(&property.second);
                    if (expired != nullptr)
                    {
                        fields.passwordExpired = *expired;
                    }
                }
                else if (property.first == "RemoteUser")
                {
                    const bool* remote = std::get_if<bool>(&property.second);
                    if (remote != nullptr)
                    {
                        fields.remote = *remote;
                    }
                }
            }
        }
        return fields;
    }

    void userAdded(sdbusplus::message::message& m)
    {
        BMCWEB_LOG_DEBUG("User Added");
        sdbusplus::message::object_path objPath;
        dbus::utility::DBusInteracesMap interfacesProperties;

        try
        {
            m.read(objPath, interfacesProperties);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR(
                "Failed to parse user add signal.ERROR={}REPLY_SIG={}",
                e.what(), m.get_signature());
            return;
        }
        BMCWEB_LOG_DEBUG("obj path = {}", objPath.str);

        std::string name = objPath.filename();
        if (name.empty())
        {
            return;
        }
        UserFields role = extractUserRole(interfacesProperties);

        // Insert the newly added user name and the role
        auto res = roleMap.emplace(name, role);
        if (!res.second)
        {
            BMCWEB_LOG_ERROR(
                "Insertion of the user=\"{}\" in the roleMap failed.", name);
            return;
        }
    }

    void userRemoved(sdbusplus::message::message& m)
    {
        BMCWEB_LOG_DEBUG("User Removed");
        sdbusplus::message::object_path objPath;

        try
        {
            m.read(objPath);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR("Failed to parse user delete signal.");
            BMCWEB_LOG_ERROR("ERROR={}REPLY_SIG={}", e.what(),
                             m.get_signature());
            return;
        }

        BMCWEB_LOG_DEBUG("obj path = {}", objPath.str);

        std::string name = objPath.filename();
        if (name.empty())
        {
            return;
        }

        roleMap.erase(name);
    }

    void userPropertiesChanged(sdbusplus::message::message& m)
    {
        BMCWEB_LOG_DEBUG("Properties Changed");
        std::string interface;
        dbus::utility::DBusPropertiesMap changedProperties;
        try
        {
            m.read(interface, changedProperties);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR("Failed to parse user properties changed signal.");
            BMCWEB_LOG_ERROR("ERROR={}REPLY_SIG={}", e.what(),
                             m.get_signature());
            return;
        }
        dbus::utility::DBusInteracesMap map;
        map.emplace_back("xyz.openbmc_project.User.Attributes",
                         changedProperties);
        const sdbusplus::message::object_path path(m.get_path());

        BMCWEB_LOG_DEBUG("Object Path = \"{}\"", path.str);

        std::string user = path.filename();
        if (user.empty())
        {
            return;
        }

        BMCWEB_LOG_DEBUG("User Name = \"{}\"", user);

        UserFields role = extractUserRole(map);

        auto userProps = roleMap.find(user);
        if (userProps == roleMap.end())
        {
            BMCWEB_LOG_CRITICAL("User {} not found", user);
            return;
        }
        if (role.userRole)
        {
            userProps->second.userRole = role.userRole;
        }
        if (role.remote)
        {
            userProps->second.remote = role.remote;
        }
        if (role.userGroups)
        {
            userProps->second.userGroups = role.userGroups;
        }
        if (role.passwordExpired)
        {
            userProps->second.passwordExpired = role.passwordExpired;
        }
    }

    void onGetManagedObjects(
        const boost::system::error_code& ec,
        const dbus::utility::ManagedObjectType& managedObjects)
    {
        if (ec)
        {
            BMCWEB_LOG_DEBUG("User manager call failed, ignoring");
            return;
        }

        for (const auto& managedObj : managedObjects)
        {
            std::string name =
                sdbusplus::message::object_path(managedObj.first).filename();
            if (name.empty())
            {
                continue;
            }
            UserFields role = extractUserRole(managedObj.second);
            roleMap.emplace(name, role);
        }
    }

    static constexpr const char* userObjPath = "/xyz/openbmc_project/user";

    UserRoleMap() :
        userAddedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesAdded(userObjPath),
            std::bind_front(&UserRoleMap::userAdded, this)),
        userRemovedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesRemoved(userObjPath),
            std::bind_front(&UserRoleMap::userRemoved, this)),
        userPropertiesChangedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::propertiesChangedNamespace(
                userObjPath, "xyz.openbmc_project.User.Attributes"),
            std::bind_front(&UserRoleMap::userPropertiesChanged, this))
    {
        dbus::utility::getManagedObjects(
            "xyz.openbmc_project.User.Manager", {userObjPath},
            std::bind_front(&UserRoleMap::onGetManagedObjects, this));
    }

    // Map of username -> role
    boost::container::flat_map<std::string, UserFields, std::less<>> roleMap;

    // These MUST be last, otherwise destruction can cause race conditions.
    sdbusplus::bus::match_t userAddedSignal;
    sdbusplus::bus::match_t userRemovedSignal;
    sdbusplus::bus::match_t userPropertiesChangedSignal;
};

} // namespace crow
