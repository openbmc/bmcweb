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
inline void findMatchedServices(const std::string& serviceName,
                                Callback&& callback)
{
    crow::connections::systemBus->async_method_call(
        [serviceName,
         callback](const boost::system::error_code ec,
                   const dbus::utility::ManagedObjectType& objects) {
        if (ec)
        {
            callback(FindError::DBusError, nullptr);
            return;
        }

        bool serviceFound = false;
        for (const auto& object : objects)
        {
            if (!matchService(object.first, serviceName))
            {
                continue;
            }

            serviceFound = true;
            callback(FindError::Ok, &object);
        }

        if (!serviceFound)
        {
            callback(FindError::NotFound, nullptr);
        }
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

void setEnabledAndRunning(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const dbus::utility::ManagedObjectType::value_type* object, bool enabled)
{
    auto errorCallback = [asyncResp](const boost::system::error_code ec) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }
    };
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus,
        "xyz.openbmc_project.Control.Service.Manager", object->first,
        "xyz.openbmc_project.Control.Service.Attributes", "Running", enabled,
        errorCallback);
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus,
        "xyz.openbmc_project.Control.Service.Manager", object->first,
        "xyz.openbmc_project.Control.Service.Attributes", "Enabled", enabled,
        errorCallback);
}
} // namespace details

void setEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& propertyName, const std::string& serviceName,
                const bool enabled)
{
    details::findMatchedServices(
        serviceName,
        [asyncResp, propertyName,
         enabled](const details::FindError error,
                  const dbus::utility::ManagedObjectType::value_type* object) {
        if (error == details::FindError::DBusError)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        if (error == details::FindError::NotFound)
        {
            // The Redfish property will not be populated in if service is not
            // found, return PropertyUnknown for PATCH request
            messages::propertyUnknown(asyncResp->res, propertyName);
            return;
        }

        details::setEnabledAndRunning(asyncResp, object, enabled);
        });
}

} // namespace service_util
} // namespace redfish
