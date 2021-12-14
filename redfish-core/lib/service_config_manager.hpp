#pragma once

#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "openbmc_dbus_rest.hpp"
#include "redfish_util.hpp"

namespace redfish
{

namespace service
{

template <typename CallbackFunc>
void setEnabled(const std::string& serviceName, const bool enabled,
                CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [serviceName, enabled,
         callback](const boost::system::error_code ec,
                   const crow::openbmc_mapper::GetSubTreeType& subtree) {
            if (ec)
            {
                callback(ec);
                return;
            }

            const std::string serviceBasePath =
                "/xyz/openbmc_project/control/service/" + serviceName;
            for (const auto& entry : subtree)
            {
                if (boost::algorithm::starts_with(entry.first, serviceBasePath))
                {
                    crow::connections::systemBus->async_method_call(
                        [callback](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                callback(ec2);
                                return;
                            }
                        },
                        entry.second.begin()->first, entry.first,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Control.Service.Attributes",
                        "Running", dbus::utility::DbusVariantType{enabled});

                    crow::connections::systemBus->async_method_call(
                        [callback](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                callback(ec2);
                                return;
                            }
                        },
                        entry.second.begin()->first, entry.first,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Control.Service.Attributes",
                        "Enabled", dbus::utility::DbusVariantType{enabled});
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/control/service", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Control.Service.Attributes"});
}

} // namespace service

} // namespace redfish
