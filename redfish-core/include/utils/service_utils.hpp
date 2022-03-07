#pragma once

#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "openbmc_dbus_rest.hpp"

#include <nlohmann/json.hpp>

namespace redfish
{
namespace service_util
{

static bool matchService(const sdbusplus::message::object_path& objPath,
                         const std::string& serviceName)
{
    // For service named as <unitName>@<instanceName>, only compare the unitName
    // part. Object path is automatically decoded as it is encoded by sdbusplus.
    std::string fullUnitName = objPath.filename();
    size_t pos = fullUnitName.rfind('@');
    return fullUnitName.substr(0, pos) == serviceName;
}

template <typename T>
static inline bool
    getPropertyFromMap(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const nlohmann::json::json_pointer& valueJsonPtr,
                       const dbus::utility::DBusPropertiesMap& properties,
                       const std::string& propertyName)
{
    for (const auto& [key, val] : properties)
    {
        if (key != propertyName)
        {
            continue;
        }

        const auto* value = std::get_if<T>(&val);
        if (value == nullptr)
        {
            messages::internalError(asyncResp->res);
            return true;
        }

        // For bool (Enabled), treat false as a invalid value
        if constexpr (std::is_same_v<bool, T>)
        {
            if (*value)
            {
                asyncResp->res.jsonValue[valueJsonPtr] = true;
                return true;
            }
        }

        // For uint16_t (Port), 0 is a invalid value
        if constexpr (std::is_same_v<uint16_t, T>)
        {
            if (*value != 0)
            {
                asyncResp->res.jsonValue[valueJsonPtr] = *value;
                return true;
            }
        }
    }
    return false;
}

void getEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& serviceName,
                const nlohmann::json::json_pointer& valueJsonPtr)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceName,
         valueJsonPtr](const boost::system::error_code ec,
                       const dbus::utility::ManagedObjectType& objects) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [path, interfaces] : objects)
        {
            if (!matchService(path, serviceName))
            {
                continue;
            }

            for (const auto& [interface, properties] : interfaces)
            {
                if (interface !=
                    "xyz.openbmc_project.Control.Service.Attributes")
                {
                    continue;
                }

                // If one of the service instance is running, show it as
                // Enabled in redfish.
                if (getPropertyFromMap<bool>(asyncResp, valueJsonPtr,
                                             properties, "Running"))
                {
                    return;
                }
            }
        }
        // If the service is not exist, simply return false
        asyncResp->res.jsonValue[valueJsonPtr] = false;
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

void getPortNumber(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& serviceName,
                   const nlohmann::json::json_pointer& valueJsonPtr)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceName,
         valueJsonPtr](const boost::system::error_code ec,
                       const dbus::utility::ManagedObjectType& objects) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [path, interfaces] : objects)
        {
            if (!matchService(path, serviceName))
            {
                continue;
            }

            for (const auto& [interface, properties] : interfaces)
            {
                if (interface !=
                    "xyz.openbmc_project.Control.Service.SocketAttributes")
                {
                    continue;
                }

                // For service with multiple instances, return the port of first
                // instance found as redfish only support one port value, they
                // should be same
                if (getPropertyFromMap<uint16_t>(asyncResp, valueJsonPtr,
                                                 properties, "Port"))
                {
                    return;
                }
            }
        }
        asyncResp->res.jsonValue[valueJsonPtr] = 0;
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

template <typename T>
static inline void
    setProperty(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& path, const std::string& interface,
                const std::string& property, T value)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        },
        "xyz.openbmc_project.Control.Service.Manager", path,
        "org.freedesktop.DBus.Properties", "Set", interface, property,
        dbus::utility::DbusVariantType{value});
}

void setEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& serviceName, const bool enabled)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceName,
         enabled](const boost::system::error_code ec,
                  const dbus::utility::ManagedObjectType& objects) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        bool serviceFound = false;
        for (const auto& [path, _] : objects)
        {
            if (matchService(path, serviceName))
            {
                serviceFound = true;
                setProperty(asyncResp, path,
                            "xyz.openbmc_project.Control.Service.Attributes",
                            "Running", enabled);
                setProperty(asyncResp, path,
                            "xyz.openbmc_project.Control.Service.Attributes",
                            "Enabled", enabled);
            }
        }

        if (!serviceFound)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

} // namespace service_util
} // namespace redfish
