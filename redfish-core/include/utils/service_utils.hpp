#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/types.hpp>

#include <string>
#include <string_view>

namespace redfish
{
namespace service_util
{
namespace details
{

bool matchService(const sdbusplus::message::object_path& objPath,
                  const std::string_view serviceName)
{
    // For service named as <unitName>@<instanceName>, only compare the unitName
    // part. Object path is automatically decoded as it is encoded by sdbusplus.
    std::string fullUnitName = objPath.filename();
    size_t pos = fullUnitName.rfind('@');
    return std::string_view(fullUnitName).substr(0, pos) == serviceName;
}

enum class FindError
{
    Ok,
    NotFound,
    DBusError,
};

template <typename Callback>
inline void findMatchedServicePaths(const std::string& serviceName,
                                    Callback&& callback)
{
    crow::connections::systemBus->async_method_call(
        [serviceName,
         callback](const boost::system::error_code ec,
                   const dbus::utility::ManagedObjectType& objects) {
        if (ec)
        {
            callback(FindError::DBusError, "");
            return;
        }

        bool serviceFound = false;
        for (const auto& [path, _] : objects)
        {
            if (!matchService(path, serviceName))
            {
                continue;
            }

            serviceFound = true;
            callback(FindError::Ok, path);
        }

        if (!serviceFound)
        {
            callback(FindError::NotFound, "");
        }
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

template <typename T>
inline T
    getPropertyFromInterface(const dbus::utility::DBusPropertiesMap& properties,
                             const std::string& propertyName)
{
    for (const auto& [key, val] : properties)
    {
        if (key != propertyName)
        {
            continue;
        }

        const auto* value = std::get_if<T>(&val);
        if (value != nullptr)
        {
            return *value;
        }
    }
    return T{};
}

} // namespace details

void getServiceProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& serviceName,
                          const nlohmann::json::json_pointer& enabledPtr,
                          const nlohmann::json::json_pointer& portPtr)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceName, enabledPtr,
         portPtr](const boost::system::error_code ec,
                  const dbus::utility::ManagedObjectType& objects) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        bool serviceFound = false;
        bool enabled = false;
        uint16_t port = 0;

        for (const auto& [path, interfaces] : objects)
        {
            if (!details::matchService(path, serviceName))
            {
                continue;
            }

            serviceFound = true;
            // For service with multiple instances, use the value of the first
            // instances found as redfish only supports one, they should be same
            for (const auto& [interface, properties] : interfaces)
            {
                if (!enabled &&
                    interface ==
                        "xyz.openbmc_project.Control.Service.Attributes")
                {
                    enabled = details::getPropertyFromInterface<bool>(
                        properties, "Running");
                }

                if ((port == 0) &&
                    interface ==
                        "xyz.openbmc_project.Control.Service.SocketAttributes")
                {
                    port = details::getPropertyFromInterface<uint16_t>(
                        properties, "Port");
                }
            }
        }

        if (serviceFound)
        {
            asyncResp->res.jsonValue[enabledPtr] = enabled;
            // Port is optional
            if (port != 0)
            {
                asyncResp->res.jsonValue[portPtr] = port;
            }
        }
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

void setEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& propertyName, const std::string& serviceName,
                const bool enabled)
{
    details::findMatchedServicePaths(
        serviceName,
        [asyncResp, propertyName, enabled](const details::FindError error,
                                           const std::string& objectPath) {
        if (error == details::FindError::DBusError)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        if (error == details::FindError::NotFound)
        {
            // The Redfish property will not be populated in if service is
            // not found, return PropertyUnknown for PATCH request
            messages::propertyUnknown(asyncResp->res, propertyName);
            return;
        }

        auto errorCallback = [asyncResp](const boost::system::error_code ec) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        };
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus,
            "xyz.openbmc_project.Control.Service.Manager", objectPath,
            "xyz.openbmc_project.Control.Service.Attributes", "Running",
            enabled, errorCallback);
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus,
            "xyz.openbmc_project.Control.Service.Manager", objectPath,
            "xyz.openbmc_project.Control.Service.Attributes", "Enabled",
            enabled, errorCallback);
        });
}

} // namespace service_util
} // namespace redfish
