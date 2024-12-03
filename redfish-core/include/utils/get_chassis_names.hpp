#pragma once

#include <dbus_singleton.hpp>

#include <array>
#include <string>
#include <vector>

namespace redfish
{

namespace utils
{

template <typename F>
inline void getChassisNames(F&& cb)
{
    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    crow::connections::systemBus->async_method_call(
        [callback = std::forward<F>(
             cb)](const boost::system::error_code ec,
                  const std::vector<std::string>& chassis) {
            std::vector<std::string> chassisNames;

            if (ec)
            {
                callback(ec, chassisNames);
                return;
            }

            chassisNames.reserve(chassis.size());
            for (const std::string& path : chassis)
            {
                sdbusplus::message::object_path dbusPath = path;
                std::string name = dbusPath.filename();
                if (name.empty())
                {
                    callback(boost::system::errc::make_error_code(
                                 boost::system::errc::invalid_argument),
                             chassisNames);
                    return;
                }
                chassisNames.emplace_back(std::move(name));
            }

            callback(ec, chassisNames);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

} // namespace utils

} // namespace redfish
