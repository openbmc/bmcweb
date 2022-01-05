#pragma once

#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "openbmc_dbus_rest.hpp"

#include <boost/container/flat_map.hpp>
#include <nlohmann/json.hpp>

namespace redfish
{
namespace service_util
{
static constexpr const char* serviceManagerService =
    "xyz.openbmc_project.Control.Service.Manager";
static constexpr const char* serviceManagerPath =
    "/xyz/openbmc_project/control/service";
static constexpr const char* serviceConfigInterface =
    "xyz.openbmc_project.Control.Service.Attributes";
static constexpr const char* portConfigInterface =
    "xyz.openbmc_project.Control.Service.SocketAttributes";

static bool matchService(const sdbusplus::message::object_path& objPath,
                         const std::string& serviceName)
{
    // For service named as <unitName>@<instanceName>, only compare the unitName
    // part. In DBus object path, '@' is escaped as "_40"
    // service-config-manager's object path is NOT encoded with sdbusplus, so
    // here we have to use the hardcoded "_40" to match
    std::string fullUnitName = objPath.filename();
    size_t pos = fullUnitName.find("_40");
    return fullUnitName.substr(0, pos) == serviceName;
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
                if (matchService(path, serviceName))
                {
                    for (const auto& [interface, properties] : interfaces)
                    {
                        if (interface != serviceConfigInterface)
                        {
                            continue;
                        }

                        for (const auto& [key, val] : properties)
                        {
                            // Service is enabled if one instance is running or
                            // enabled
                            if (key == "Enabled" || key == "Running")
                            {
                                const auto* enabled = std::get_if<bool>(&val);
                                if (enabled == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                if (*enabled)
                                {
                                    asyncResp->res.jsonValue[valueJsonPtr] =
                                        true;
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            // If the service is not exist, simply return false
            asyncResp->res.jsonValue[valueJsonPtr] = false;
        },
        serviceManagerService, serviceManagerPath,
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
                if (matchService(path, serviceName))
                {
                    for (const auto& [interface, properties] : interfaces)
                    {
                        if (interface != portConfigInterface)
                        {
                            continue;
                        }

                        for (const auto& [key, val] : properties)
                        {
                            // For service with multiple instances, return the
                            // port of first instance found as redfish only
                            // support one port value, they should be same
                            if (key == "Port")
                            {
                                const auto* port = std::get_if<uint16_t>(&val);
                                if (port == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                asyncResp->res.jsonValue[valueJsonPtr] = *port;
                                return;
                            }
                        }
                    }
                }
            }
            asyncResp->res.jsonValue[valueJsonPtr] =
                nlohmann::json::value_t::null;
        },
        serviceManagerService, serviceManagerPath,
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
        serviceManagerService, path, "org.freedesktop.DBus.Properties", "Set",
        interface, property, dbus::utility::DbusVariantType{value});
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

            for (const auto& [path, _] : objects)
            {
                if (matchService(path, serviceName))
                {
                    setProperty(asyncResp, path, serviceConfigInterface,
                                "Running", enabled);
                    setProperty(asyncResp, path, serviceConfigInterface,
                                "Enabled", enabled);
                }
            }
        },
        serviceManagerService, serviceManagerPath,
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

void setPortNumber(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& serviceName, const uint16_t portNumber)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceName,
         portNumber](const boost::system::error_code ec,
                     const dbus::utility::ManagedObjectType& objects) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& [path, _] : objects)
            {
                if (matchService(path, serviceName))
                {
                    setProperty(asyncResp, path, portConfigInterface, "Port",
                                portNumber);
                }
            }
        },
        serviceManagerService, serviceManagerPath,
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

} // namespace service_util
} // namespace redfish
