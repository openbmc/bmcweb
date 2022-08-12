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
static constexpr std::string_view sshServiceName = "dropbear";
static constexpr std::string_view httpsServiceName = "bmcweb";
static constexpr std::string_view ipmiServiceName = "phosphor-ipmi-net";

static constexpr std::array<std::pair<std::string_view, std::string_view>, 3>
    networkProtocolToDbus = {{{"SSH", sshServiceName},
                              {"HTTPS", httpsServiceName},
                              {"IPMI", ipmiServiceName}}};

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

inline void afterGetServiceProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code ec,
    const dbus::utility::ManagedObjectType& objects)
{
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [path, interfaces] : objects)
    {
        for (const auto& [jsonPropName, serviceName] : networkProtocolToDbus)
        {
            if (!details::matchService(path, serviceName))
            {
                continue;
            }

            // For service with multiple instances, use the value of the
            // first instances found as redfish only supports one, they
            // should be same
            for (const auto& [interface, properties] : interfaces)
            {
                if (interface ==
                    "xyz.openbmc_project.Control.Service.Attributes")
                {
                    bool running = false;
                    sdbusplus::unpackPropertiesNoThrow(
                        dbus_utils::UnpackErrorPrinter(), properties, "Running",
                        running);

                    asyncResp->res.jsonValue[jsonPropName]["ProtocolEnabled"] =
                        running;
                }

                if (interface ==
                    "xyz.openbmc_project.Control.Service.SocketAttributes")
                {
                    uint16_t port = 0;
                    sdbusplus::unpackPropertiesNoThrow(
                        dbus_utils::UnpackErrorPrinter(), properties, "Port",
                        port);
                    // Port is optional
                    if (port != 0)
                    {
                        asyncResp->res.jsonValue[jsonPropName]["Port"] = port;
                    }
                }
            }
        }
    }
}

inline void
    getServiceProperties(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::ManagedObjectType& objects) {
        afterGetServiceProperties(asyncResp, ec, objects);
    },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

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
