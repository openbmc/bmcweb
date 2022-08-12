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

template <typename Callback>
void afterFindMatchedServicePaths(
    std::string_view serviceName, Callback&& callback,
    const boost::system::error_code& ec,
    const dbus::utility::ManagedObjectType& objects)
{
    std::vector<std::string> servicePaths;
    if (!ec)
    {
        for (const auto& [path, _] : objects)
        {
            if (matchService(path, serviceName))
            {
                servicePaths.emplace_back(path);
            }
        }
    }

    callback(ec, servicePaths);
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
                            const boost::system::error_code& ec,
                            const std::vector<std::string>& objectPaths)
{
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    for (const std::string& objectPath : objectPaths)
    {
        if (objectPaths.empty())
        {
            // The Redfish property will not be populated in if service is
            // not found, return PropertyUnknown for PATCH request
            messages::propertyUnknown(asyncResp->res, propertyName);
            return;
        }

        sdbusplus::asio::setProperty(
            *crow::connections::systemBus,
            "xyz.openbmc_project.Control.Service.Manager", objectPath,
            "xyz.openbmc_project.Control.Service.Attributes", "Running",
            enabled, [asyncResp](const boost::system::error_code& ec2) {
            if (ec2)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        });
        sdbusplus::asio::setProperty(
            *crow::connections::systemBus,
            "xyz.openbmc_project.Control.Service.Manager", objectPath,
            "xyz.openbmc_project.Control.Service.Attributes", "Enabled",
            enabled, [asyncResp](const boost::system::error_code& ec2) {
            if (ec2)
            {
                messages::internalError(asyncResp->res);
                return;
            }
        });
    }
}

inline void setEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       std::string_view propertyName,
                       std::string_view serviceName, bool enabled)
{
    details::findMatchedServicePaths(
        serviceName, [asyncResp, propertyName{std::string(propertyName)},
                      enabled](const boost::system::error_code& ec,
                               const std::vector<std::string>& objectPaths) {
        afterSetEnabled(asyncResp, propertyName, enabled, ec, objectPaths);
    });
}

} // namespace service_util
} // namespace redfish
