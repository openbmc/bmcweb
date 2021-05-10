#pragma once

//#include "dbus_utility.hpp"
#include "logging.hpp"

#include <boost/url/format.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/unpack_properties.hpp>
#include <sdbusplus/bus/match.hpp>

#include <memory>
#include <vector>

namespace crow
{
constexpr const char* userService = "xyz.openbmc_project.User.Manager";
constexpr const char* userObjPath = "/xyz/openbmc_project/user";
constexpr const char* userAttrIface = "xyz.openbmc_project.User.Attributes";
constexpr const char* dbusPropertiesIface = "org.freedesktop.DBus.Properties";

struct UserRoleMap
{
    using GetManagedPropertyType =
        boost::container::flat_map<std::string,
                                   std::variant<std::string, bool>>;

    using InterfacesPropType =
        boost::container::flat_map<std::string, GetManagedPropertyType>;

    using GetManagedObjectsType = std::vector<
        std::pair<sdbusplus::message::object_path, InterfacesPropType>>;

    static UserRoleMap& getInstance()
    {
        static UserRoleMap userRoleMap;
        return userRoleMap;
    }

    UserRoleMap(const UserRoleMap&) = delete;
    UserRoleMap& operator=(const UserRoleMap&) = delete;

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

    void insertLDAPUserRoleFromDbus(const std::string& userName,
                                    const std::string& role)
    {
        if (userName.empty() || role.empty())
        {
            return;
        }

        auto res = roleMap.emplace(userName, role);
        if (res.second == false)
        {
            BMCWEB_LOG_ERROR << "Insertion of the user=\"" << userName
                             << "\" in the roleMap failed.";
            return;
        }
    }

    std::string extractUserRole(const InterfacesPropType& interfacesProperties)
    {
        auto iface = interfacesProperties.find(userAttrIface);
        if (iface == interfacesProperties.end())
        {
            return {};
        }

        auto& properties = iface->second;
        auto property = properties.find("UserPrivilege");
        if (property == properties.end())
        {
            return {};
        }

        const std::string* role = std::get_if<std::string>(&property->second);
        if (role == nullptr)
        {
            BMCWEB_LOG_ERROR << "UserPrivilege property value is null";
            return {};
        }

        return *role;
    }

  private:
    void userAdded(sdbusplus::message::message& m)
    {
        BMCWEB_LOG_DEBUG << "User Added";
        sdbusplus::message::object_path objPath;
        InterfacesPropType interfacesProperties;

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
        if (res.second == false)
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
        GetManagedPropertyType changedProperties;
        m.read(interface, changedProperties);
        const sdbusplus::message::object_path path(m.get_path());

        BMCWEB_LOG_DEBUG << "Object Path = \"" << path.str << "\"";

        std::string user = path.filename();
        if (user.empty())
        {
            return;
        }

        BMCWEB_LOG_DEBUG << "User Name = \"" << user << "\"";

        auto index = changedProperties.find("UserPrivilege");
        if (index == changedProperties.end())
        {
            return;
        }

        const std::string* role = std::get_if<std::string>(&index->second);
        if (role == nullptr)
        {
            return;
        }
        BMCWEB_LOG_DEBUG << "Role = \"" << *role << "\"";

        auto it = roleMap.find(user);
        if (it == roleMap.end())
        {
            BMCWEB_LOG_ERROR << "User Name = \"" << user
                             << "\" is not found. But, received "
                                "propertiesChanged signal";
            return;
        }
        it->second = *role;
    }

    UserRoleMap() :
        userAddedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesAdded(userObjPath),
            [this](sdbusplus::message::message& m) { userAdded(m); }),
        userRemovedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::interfacesRemoved(userObjPath),
            [this](sdbusplus::message::message& m) { userRemoved(m); }),
        userPropertiesChangedSignal(
            *crow::connections::systemBus,
            sdbusplus::bus::match::rules::path_namespace(userObjPath) +
                sdbusplus::bus::match::rules::type::signal() +
                sdbusplus::bus::match::rules::member("PropertiesChanged") +
                sdbusplus::bus::match::rules::interface(dbusPropertiesIface) +
                sdbusplus::bus::match::rules::argN(0, userAttrIface),
            [this](sdbusplus::message::message& m) {
        userPropertiesChanged(m);
            })
    {
        crow::connections::systemBus->async_method_call(
            [this](const boost::system::error_code& ec,
                   GetManagedObjectsType& managedObjects) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "User manager call failed, ignoring";
                return;
            }

            for (auto& managedObj : managedObjects)
            {
                std::size_t lastPos = managedObj.first.str.rfind('/');
                if (lastPos == std::string::npos)
                {
                    continue;
                };
                std::string name = managedObj.first.str.substr(lastPos + 1);
                std::string role = extractUserRole(managedObj.second);
                roleMap.emplace(name, role);
            }
            },
            userService, userObjPath, "org.freedesktop.DBus.ObjectManager",
            "GetManagedObjects");
    }

    boost::container::flat_map<std::string, std::string, std::less<>> roleMap;
    // These MUST be last, otherwise destruction can cause race conditions.
    sdbusplus::bus::match_t userAddedSignal;
    sdbusplus::bus::match_t userRemovedSignal;
    sdbusplus::bus::match_t userPropertiesChangedSignal;
};

} // namespace crow
