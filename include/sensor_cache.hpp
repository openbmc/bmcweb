#pragma once

#include "dbus_singleton.hpp"
#include "property_tree.hpp"

#include <boost/container/flat_map.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <cstdint>

namespace dbus
{

struct PropertyCache
{
  private:
    std::vector<sdbusplus::bus::match_t> matches;

    PropertyTree cache;

    PropertyCache()
    {
        monitorNamespace("/xyz/openbmc_project/sensors");
        monitorNamespace("/xyz/openbmc_project/inventory");
    }
    void monitorNamespace(std::string_view ns)
    {
        std::string matchExpr = std::format(
            "path_namespace='{}',type=signal,interface=org.freedesktop.DBus,member=PropertiesChanged",
            ns);
        matches.emplace_back(
            *crow::connections::systemBus, matchExpr,
            std::bind_front(&PropertyCache::onPropertiesChanged, this));

        matchExpr = std::format(
            "path_namespace='{}',type=signal,interface=org.freedesktop.DBus.ObjectManager,member=InterfacesAdded",
            ns);
        matches.emplace_back(
            *crow::connections::systemBus, matchExpr,
            std::bind_front(&PropertyCache::onInterfacesAdded, this));
        matchExpr = std::format(
            "path_namespace='{}',type=signal,interface=org.freedesktop.DBus.ObjectManager,member=InterfacesRemoved",
            ns);
        matches.emplace_back(
            *crow::connections::systemBus, matchExpr,
            std::bind_front(&PropertyCache::onInterfacesRemoved, this));
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

        for (const auto& interface : interfacesProperties)
        {
            for (const auto& value : interface.second)
            {
                const double* sensorValue = std::get_if<double>(&value.second);
                if (sensorValue != nullptr)
                {
                    cache.setDoubleValue(dbusPath, value.first, *sensorValue);
                    continue;
                }
                const std::string* strValue =
                    std::get_if<std::string>(&value.second);
                if (strValue != nullptr)
                {
                    cache.setStringValue(dbusPath, value.first, *strValue);
                    continue;
                }
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

        for (const auto& value : map)
        {
            const double* sensorValue = std::get_if<double>(&value.second);
            if (sensorValue != nullptr)
            {
                cache.setDoubleValue(dbusPath, value.first, *sensorValue);
                continue;
            }
            const std::string* strValue =
                std::get_if<std::string>(&value.second);
            if (strValue != nullptr)
            {
                cache.setStringValue(dbusPath, value.first, *strValue);
                continue;
            }
        }

        for (const std::string& property : invalidatedProperties)
        {
            cache.eraseValue(dbusPath, property);
        }
    }

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
        // TODO implement interface removal
    }

    utility::DBusPropertiesMap getAllProperties(const std::string& service,
                                                const std::string& objectPath,
                                                const std::string& interface)
    {
        return cache.getAllProperties(service, objectPath, interface);
    }

    utility::DbusVariantType getAllProperties(
        const std::string& service, const std::string& objectPath,
        const std::string& interface, const std::string& propName)
    {
        return cache.getProperty(service, objectPath, interface, propName);
    }

  public:
    static PropertyCache& getInstance()
    {
        static PropertyCache sc;
        return sc;
    }

    std::span<const std::pair<std::string, double>> getSensorValues()
    {
        return {};
    }
};

} // namespace dbus
