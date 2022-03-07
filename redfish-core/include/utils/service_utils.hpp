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

static bool matchService(const sdbusplus::message::object_path& objPath,
                         const std::string& serviceName)
{
    // For service named as <unitName>@<instanceName>, only compare the unitName
    // part. In DBus object path, '@' is escaped as "_40"
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
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& entry : subtree)
            {
                if (matchService(entry.first, serviceName))
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp, serviceName, valueJsonPtr](
                            const boost::system::error_code ec2,
                            const boost::container::flat_map<
                                std::string, dbus::utility::DbusVariantType>&
                                properties) {
                            if (ec2)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            bool enabled = false;
                            bool running = false;
                            for (const auto& [key, val] : properties)
                            {
                                if (key == "Enabled")
                                {
                                    const auto* value = std::get_if<bool>(&val);
                                    if (value == nullptr)
                                    {
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    enabled = *value;
                                }
                                else if (key == "Running")
                                {
                                    const auto* value = std::get_if<bool>(&val);
                                    if (value == nullptr)
                                    {
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    running = *value;
                                }
                            }
                            asyncResp->res.jsonValue[valueJsonPtr] =
                                (enabled || running);
                        },
                        entry.second.begin()->first, entry.first,
                        "org.freedesktop.DBus.Properties", "GetAll",
                        "xyz.openbmc_project.Control.Service.Attributes");
                    return;
                }
            }
            asyncResp->res.jsonValue[valueJsonPtr] =
                nlohmann::json::value_t::null;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/control/service", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Control.Service.Attributes"});
}

void getPortNumber(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& serviceName,
                   const nlohmann::json::json_pointer& valueJsonPtr)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceName,
         valueJsonPtr](const boost::system::error_code ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& entry : subtree)
            {
                if (matchService(entry.first, serviceName))
                {
                    sdbusplus::asio::getProperty<uint16_t>(
                        *crow::connections::systemBus,
                        entry.second.begin()->first, entry.first,
                        "xyz.openbmc_project.Control.Service.SocketAttributes",
                        "Port",
                        [asyncResp, serviceName,
                         valueJsonPtr](const boost::system::error_code ec2,
                                       const uint16_t portNumber) {
                            if (ec2)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            asyncResp->res.jsonValue[valueJsonPtr] = portNumber;
                        });
                    return;
                }
            }
            asyncResp->res.jsonValue[valueJsonPtr] =
                nlohmann::json::value_t::null;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/control/service", 0,
        std::array<const char*, 1>{
            "xyz.openbmc_project.Control.Service.SocketAttributes"});
}

void setEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                const std::string& serviceName, const bool enabled)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, serviceName,
         enabled](const boost::system::error_code ec,
                  const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            for (const auto& entry : subtree)
            {
                if (matchService(entry.first, serviceName))
                {
                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }
                        },
                        entry.second.begin()->first, entry.first,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Control.Service.Attributes",
                        "Running", dbus::utility::DbusVariantType{enabled});
                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code ec2) {
                            if (ec2)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }
                        },
                        entry.second.begin()->first, entry.first,
                        "org.freedesktop.DBus.Properties", "Set",
                        "xyz.openbmc_project.Control.Service.Attributes",
                        "Enabled", dbus::utility::DbusVariantType{enabled});
                    return;
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

} // namespace service_util
} // namespace redfish
