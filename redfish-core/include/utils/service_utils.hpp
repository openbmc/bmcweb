#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/types.hpp>

#include <string>

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
            if (!matchService(path, serviceName))
            {
                continue;
            }

            serviceFound = true;
            auto errorCallback =
                [asyncResp](const boost::system::error_code ec2) {
                if (ec2)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            };
            sdbusplus::asio::setProperty(
                *crow::connections::systemBus,
                "xyz.openbmc_project.Control.Service.Manager", path,
                "xyz.openbmc_project.Control.Service.Attributes", "Running",
                enabled, std::move(errorCallback));
            sdbusplus::asio::setProperty(
                *crow::connections::systemBus,
                "xyz.openbmc_project.Control.Service.Manager", path,
                "xyz.openbmc_project.Control.Service.Attributes", "Enabled",
                enabled, std::move(errorCallback));
        }

        // The Redfish property will not be populated in if service is not
        // found, return PropertyUnknown for PATCH request
        if (!serviceFound)
        {
            messages::propertyUnknown(asyncResp->res, "Enabled");
            return;
        }
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

} // namespace service_util
} // namespace redfish
