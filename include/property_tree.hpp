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
    boost::unordered_flat_map<std::string, utility::DbusVariantType, StringHash, std::equal_to<>>
        dProps;

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

    void setDoubleValue(std::string_view path, std::string_view propertyName,
                        double value)
    {
        PropertyTree& tree = getPropertyTreeForPath(path);
        tree.dProps[propertyName] = value;
    }

    void setStringValue(std::string_view path, std::string_view propertyName,
                        std::string_view value)
    {
        PropertyTree& tree = getPropertyTreeForPath(path);
        tree.dProps[propertyName].emplace<std::string>(value);
    }

    void eraseValue(std::string_view path, std::string_view propertyName)
    {
        PropertyTree& tree = getPropertyTreeForPath(path);
        tree.dProps.erase(propertyName);
    }

    std::optional<double> getDoubleValue(std::string_view path,
                                         std::string_view propertyName)
    {
        PropertyTree& tree = getPropertyTreeForPath(path);
        auto it = tree.dProps.find(propertyName);
        if (it == tree.dProps.end())
        {
            return std::nullopt;
        }
        const double* dval = std::get_if<double>(&it->second);
        if (dval == nullptr){
            return std::nullopt;
        }
        return *dval;
    }
    std::optional<std::string> getStringValue(std::string_view path,
                                              std::string_view propertyName)
    {
        PropertyTree& tree = getPropertyTreeForPath(path);
        auto it = tree.dProps.find(propertyName);
        if (it == tree.dProps.end())
        {
            return std::nullopt;
        }
        const std::string* dval = std::get_if<std::string>(&it->second);
        if (dval == nullptr){
            return std::nullopt;
        }
        return *dval;
    }

    ::dbus::utility::DBusPropertiesMap getAllProperties(
        const std::string& /*service*/, const std::string& objectPath,
        const std::string& /*interface*/)
    {
        utility::DBusPropertiesMap map;
        PropertyTree& tree = getPropertyTreeForPath(objectPath);

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
        PropertyTree& tree = getPropertyTreeForPath(objectPath);
        auto it = tree.dProps.find(propName);
        if (it != tree.dProps.end())
        {
            return it->second;
        }
        return {};
    }

};

} // namespace dbus
