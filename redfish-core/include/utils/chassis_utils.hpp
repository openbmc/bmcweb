#pragma once

#include <dbus_utility.hpp>

namespace redfish
{

namespace chassis_utils
{

constexpr std::array<const char*, 2> dBusChassisInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Board",
    "xyz.openbmc_project.Inventory.Item.Chassis",
};

/**
 * @brief Retrieves valid chassis ID
 * @param chassisId Chassis Id to search for
 * @param callback  Callback for next step to get valid chassis ID
 */
template <typename Callback>
void getValidChassisPath(const std::string& chassisID, Callback&& callback)
{
    auto respHandler =
        [callback{std::forward<Callback>(callback)}, chassisID](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths) {
        if (!ec)
        {
            for (const std::string& chassis : chassisPaths)
            {
                sdbusplus::message::object_path path(chassis);
                std::string chassisName = path.filename();
                if (chassisName.empty())
                {
                    BMCWEB_LOG_ERROR << "Failed to find chassisName in "
                                     << chassis;
                    continue;
                }
                if (chassisName == chassisID)
                {
                    callback(ec, path);
                    return;
                }
            }
        }
        callback(ec, std::nullopt);
    };

    // Get the Chassis Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, dBusChassisInterfaces);
}

} // namespace chassis_utils
} // namespace redfish
