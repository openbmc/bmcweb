#pragma once

#include "dbus_utility.hpp"

#include <boost/container_hash/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string_view>

namespace dbus
{

struct StringHash
{
    using is_transparent = void;
    size_t operator()(const char* txt) const
    {
        return boost::hash<std::string_view>{}(txt);
    }
    size_t operator()(std::string_view txt) const
    {
        return boost::hash<std::string_view>{}(txt);
    }
    size_t operator()(const std::string& txt) const
    {
        return boost::hash<std::string>{}(txt);
    }
};

struct PropertyTree
{
    // Subpaths of this path keyed by next value of the path
    // if this /xyz
    // children.first is openbmc_project
    using Children = boost::unordered_flat_map<std::string, PropertyTree,
                                               StringHash, std::equal_to<>>;

    Children children;
    boost::unordered_flat_map<std::string, utility::DbusVariantType, StringHash,
                              std::equal_to<>>
        dProps;

    std::vector<sdbusplus::bus::match_t> matches;

    PropertyTree& getPropertyTreeForPath(std::string_view path)
    {
        if (path.empty())
        {
            return *this;
        }
        if (path[0] != '/')
        {
            return *this;
        }
        size_t index = path.find('/', 1);
        std::string_view parent = path.substr(0, index);
        std::string_view child;
        if (index != std::string_view::npos)
        {
            child = path.substr(index);
        }
        return children[parent].getPropertyTreeForPath(child);
    }

  public:
    static PropertyTree& getInstance()
    {
        static PropertyTree sc;
        return sc;
    }
};

struct DbusCache
{
    std::vector<sdbusplus::bus::match_t> matches;

    PropertyTree root;

    ::dbus::utility::DBusPropertiesMap getAllProperties(
        const std::string& /*service*/, const std::string& objectPath,
        const std::string& /*interface*/)
    {
        utility::DBusPropertiesMap map;
        PropertyTree& tree = root.getPropertyTreeForPath(objectPath);
        map.reserve(tree.dProps.size());

        for (const auto& drop : tree.dProps)
        {
            map.emplace_back(drop.first, drop.second);
        }
        return map;
    }

    ::dbus::utility::DbusVariantType getProperty(
        const std::string& /*service*/, const std::string& objectPath,
        const std::string& /*interface*/, const std::string& propName)
    {
        PropertyTree& tree = root.getPropertyTreeForPath(objectPath);
        auto it = tree.dProps.find(propName);
        if (it != tree.dProps.end())
        {
            return it->second;
        }
        return {};
    }

    DbusCache()
    {
        monitorNamespace("/xyz/openbmc_project");
    }

    void monitorNamespace(std::string_view ns)
    {
        std::string matchExpr = std::format(
            "path_namespace='{}',type=signal,interface=org.freedesktop.DBus,member=PropertiesChanged",
            ns);
        matches.emplace_back(
            *crow::connections::systemBus, matchExpr,
            std::bind_front(&DbusCache::onPropertiesChanged, this));

        matchExpr = std::format(
            "path_namespace='{}',type=signal,interface=org.freedesktop.DBus.ObjectManager,member=InterfacesAdded",
            ns);
        matches.emplace_back(
            *crow::connections::systemBus, matchExpr,
            std::bind_front(&DbusCache::onInterfacesAdded, this));
        matchExpr = std::format(
            "path_namespace='{}',type=signal,interface=org.freedesktop.DBus.ObjectManager,member=InterfacesRemoved",
            ns);
        matches.emplace_back(
            *crow::connections::systemBus, matchExpr,
            std::bind_front(&DbusCache::onInterfacesRemoved, this));
    }

    void onInterfacesAdded(sdbusplus::message::message& message)
    {
        utility::DBusInterfacesMap interfacesProperties;

        sdbusplus::message::object_path csrObjectPath;
        message.read(csrObjectPath, interfacesProperties);
        const char* path = message.get_path();
        if (path == nullptr)
        {
            BMCWEB_LOG_ERROR("Path wasn't populated?");
            return;
        }
        std::string_view dbusPath(path);

        PropertyTree& tree = root.getPropertyTreeForPath(dbusPath);
        for (const auto& interface : interfacesProperties){
            for (const auto& property : interface.second){
                tree.dProps[property.first] = property.second;
            }
        }
    }

    void onPropertiesChanged(sdbusplus::message::message& message)
    {
        std::string interface;
        utility::DBusPropertiesMap map;
        std::vector<std::string> invalidatedProperties;
        message.read(interface, map, invalidatedProperties);

        const char* path = message.get_path();
        if (path == nullptr)
        {
            BMCWEB_LOG_ERROR("Path wasn't populated?");
            return;
        }
        std::string_view dbusPath(path);
        PropertyTree& tree = root.getPropertyTreeForPath(dbusPath);
        for (const auto& value : map)
        {
            tree.dProps[value.first] = value.second;
        }

        for (const std::string& property : invalidatedProperties)
        {
            tree.dProps.erase(property);
        }
    }

    void onPropertyChanged(std::string_view path, std::string_view propertyName,
                            utility::DbusVariantType value)
    {
        PropertyTree& tree = root.getPropertyTreeForPath(path);
        tree.dProps[propertyName] = value;
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void onInterfacesRemoved(sdbusplus::message::message& message)
    {
        utility::DBusInterfacesMap interfacesProperties;

        sdbusplus::message::object_path csrObjectPath;
        message.read(csrObjectPath, interfacesProperties);
        const char* path = message.get_path();
        if (path == nullptr)
        {
            BMCWEB_LOG_ERROR("Path wasn't populated?");
            return;
        }
        std::string_view dbusPath(path);
    }

};

} // namespace dbus
