#pragma once

#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "redfish_util.hpp"

namespace redfish
{

namespace service_config
{

static bool matchService(const sdbusplus::message::object_path& objPath,
                         const std::string& serviceName)
{
    // For service named as <unitName>@<instanceName>, only compare the unitName
    // part. In DBus object path, '@' is escaped as "_40"
    std::string fullUnitName = objPath.filename();
    size_t pos = fullUnitName.find("_40");
    return fullUnitName.substr(0, pos) == serviceName;
}

template <typename CallbackFunc>
void setEnabled(const std::string& serviceName, const bool enabled,
                CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [serviceName, enabled,
         callback](const boost::system::error_code ec,
                   const dbus::utility::ManagedObjectType& objects) {
            if (ec)
            {
                callback(ec);
                return;
            }

            for (const auto& object : objects)
            {
                if (matchService(object.first, serviceName))
                {
                    crow::connections::systemBus->async_method_call(
                        [callback](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                callback(ec2);
                                return;
                            }
                        },
                        "xyz.openbmc_project.Control.Service.Manager",
                        object.first.str, "org.freedesktop.DBus.Properties",
                        "Set", "xyz.openbmc_project.Control.Service.Attributes",
                        "Running", dbus::utility::DbusVariantType{enabled});

                    crow::connections::systemBus->async_method_call(
                        [callback](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                callback(ec2);
                                return;
                            }
                        },
                        "xyz.openbmc_project.Control.Service.Manager",
                        object.first.str, "org.freedesktop.DBus.Properties",
                        "Set", "xyz.openbmc_project.Control.Service.Attributes",
                        "Enabled", dbus::utility::DbusVariantType{enabled});
                }
            }
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

template <typename CallbackFunc>
void setPortNumber(const std::string& serviceName, const uint16_t portNumber,
                   CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [serviceName, portNumber,
         callback](const boost::system::error_code ec,
                   const dbus::utility::ManagedObjectType& objects) {
            if (ec)
            {
                callback(ec);
                return;
            }

            for (const auto& object : objects)
            {
                if (matchService(object.first, serviceName))
                {
                    crow::connections::systemBus->async_method_call(
                        [callback](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                callback(ec2);
                                return;
                            }
                        },
                        "xyz.openbmc_project.Control.Service.Manager",
                        object.first.str, "org.freedesktop.DBus.Properties",
                        "Set",
                        "xyz.openbmc_project.Control.Service.SocketAttributes",
                        "Port", dbus::utility::DbusVariantType{portNumber});
                }
            }
        },
        "xyz.openbmc_project.Control.Service.Manager",
        "/xyz/openbmc_project/control/service",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

} // namespace service_config

} // namespace redfish
