#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

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

inline bool matchService(const sdbusplus::message::object_path& objPath,
                         std::string_view serviceName)
{
    // For service named as <unitName>@<instanceName>, only compare the unitName
    // part. Object path is automatically decoded as it is encoded by sdbusplus.
    std::string fullUnitName = objPath.filename();
    if (fullUnitName.empty())
    {
        return false;
    }
    size_t pos = fullUnitName.rfind('@');
    return fullUnitName.substr(0, pos) == serviceName;
}

enum class FindError
{
    Ok,
    NotFound,
    DBusError,
};

template <typename Callback>
void afterFindMatchedServicePaths(
    std::string_view serviceName, Callback&& callback,
    const boost::system::error_code& ec,
    const dbus::utility::ManagedObjectType& objects)
{
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
}

template <typename Callback>
void findMatchedServicePaths(std::string_view serviceName, Callback&& callback)
{
    crow::connections::systemBus->async_method_call(
        [serviceName{std::string(serviceName)},
         callback](const boost::system::error_code& ec,
                   const dbus::utility::ManagedObjectType& objects) {
        afterFindMatchedServicePaths(serviceName, callback, ec, objects);
    },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

} // namespace details

inline void afterSetEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            std::string_view propertyName, bool enabled,
                            const details::FindError& error,
                            const std::string& objectPath)
{
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

    auto errorCallback = [asyncResp](const boost::system::error_code& ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }
    };
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus,
        "xyz.openbmc_project.Control.Service.Manager", objectPath,
        "xyz.openbmc_project.Control.Service.Attributes", "Running", enabled,
        errorCallback);
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus,
        "xyz.openbmc_project.Control.Service.Manager", objectPath,
        "xyz.openbmc_project.Control.Service.Attributes", "Enabled", enabled,
        errorCallback);
}

inline void setEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       std::string_view propertyName,
                       std::string_view serviceName, bool enabled)
{
    details::findMatchedServicePaths(
        serviceName,
        [asyncResp, propertyName{std::string(propertyName)}, enabled](
            const details::FindError& error, const std::string& objectPath) {
        afterSetEnabled(asyncResp, propertyName, enabled, error, objectPath);
    });
}

} // namespace service_util
} // namespace redfish
