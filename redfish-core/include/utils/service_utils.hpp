#pragma once

#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "openbmc_dbus_rest.hpp"

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
                setProperty(asyncResp, path, serviceConfigInterface, "Running",
                            enabled);
                setProperty(asyncResp, path, serviceConfigInterface, "Enabled",
                            enabled);
            }
        }
        },
        serviceManagerService, serviceManagerPath,
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

} // namespace service_util
} // namespace redfish
