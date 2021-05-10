#pragma once

#include "dbus_utility.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <memory>
#include <vector>

namespace crow
{
struct UserRoleMap
{
  public:
    static UserRoleMap& getInstance()
    {
        static UserRoleMap userRoleMap;
        return userRoleMap;
    }

    std::string getUserRole(std::string_view name)
    {
        auto it = roleMap.find(name);
        if (it == roleMap.end())
        {
            BMCWEB_LOG_ERROR << "User name " << name
                             << " is not found in the UserRoleMap.";
            return "";
        }
        return it->second;
    }
    UserRoleMap(const UserRoleMap&) = delete;
    UserRoleMap& operator=(const UserRoleMap&) = delete;
    UserRoleMap(UserRoleMap&&) = delete;
    UserRoleMap& operator=(UserRoleMap&&) = delete;
    ~UserRoleMap() = default;

  private:
    static std::string extractUserRole(
        const dbus::utility::DBusInteracesMap& interfacesProperties)
    {
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
                        return *role;
                    }
                }
            }
        }
        return "";
    }

    void userAdded(sdbusplus::message::message& m)
    {
        BMCWEB_LOG_DEBUG << "User Added";
        sdbusplus::message::object_path objPath;
        dbus::utility::DBusInteracesMap interfacesProperties;

        try
        {
            m.read(objPath, interfacesProperties);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR << "Failed to parse user add signal."
                             << "ERROR=" << e.what()
                             << "REPLY_SIG=" << m.get_signature();
            return;
        }
        BMCWEB_LOG_DEBUG << "obj path = " << objPath.str;

        std::string name = objPath.filename();
        if (name.empty())
        {
            return;
        }
        std::string role = extractUserRole(interfacesProperties);

        // Insert the newly added user name and the role
        auto res = roleMap.emplace(name, role);
        if (!res.second)
        {
            BMCWEB_LOG_ERROR << "Insertion of the user=\"" << name
                             << "\" in the roleMap failed.";
            return;
        }
    }

    void userRemoved(sdbusplus::message::message& m)
    {
        BMCWEB_LOG_DEBUG << "User Removed";
        sdbusplus::message::object_path objPath;

        try
        {
            m.read(objPath);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR << "Failed to parse user delete signal.";
            BMCWEB_LOG_ERROR << "ERROR=" << e.what()
                             << "REPLY_SIG=" << m.get_signature();
            return;
        }

        BMCWEB_LOG_DEBUG << "obj path = " << objPath.str;

        std::string name = objPath.filename();
        if (name.empty())
        {
            return;
        }

        roleMap.erase(name);
    }

    void userPropertiesChanged(sdbusplus::message::message& m)
    {
        BMCWEB_LOG_DEBUG << "Properties Changed";
        std::string interface;
        dbus::utility::DBusPropertiesMap changedProperties;
        try
        {
            m.read(interface, changedProperties);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            BMCWEB_LOG_ERROR
                << "Failed to parse user properties changed signal.";
            BMCWEB_LOG_ERROR << "ERROR=" << e.what()
                             << "REPLY_SIG=" << m.get_signature();
            return;
        }
        const sdbusplus::message::object_path path(m.get_path());

        BMCWEB_LOG_DEBUG << "Object Path = \"" << path.str << "\"";

        std::string user = path.filename();
        if (user.empty())
        {
            return;
        }

        BMCWEB_LOG_DEBUG << "User Name = \"" << user << "\"";

        for (const auto& property : changedProperties)
        {
            if (property.first != "UserPrivilege")
            {
                continue;
            }
            auto it = roleMap.find(user);
            if (it == roleMap.end())
            {
                BMCWEB_LOG_ERROR << "User Name = \"" << user
                                 << "\" is not found. But, received "
                                    "propertiesChanged signal";
                return;
            }
            const std::string* role =
                std::get_if<std::string>(&property.second);
            if (role == nullptr)
            {
                continue;
            }
            BMCWEB_LOG_DEBUG << "Role = \"" << *role << "\"";
            it->second = *role;
            return;
        }
    }

    void onGetManagedObjects(
        const boost::system::error_code& ec,
        const dbus::utility::ManagedObjectType& managedObjects)
    {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "User manager call failed, ignoring";
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
            std::string role = extractUserRole(managedObj.second);
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
    boost::container::flat_map<std::string, std::string, std::less<>> roleMap;

    // These MUST be last, otherwise destruction can cause race conditions.
    sdbusplus::bus::match_t userAddedSignal;
    sdbusplus::bus::match_t userRemovedSignal;
    sdbusplus::bus::match_t userPropertiesChangedSignal;
};

} // namespace crow
