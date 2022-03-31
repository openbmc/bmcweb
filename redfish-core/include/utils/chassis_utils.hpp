#pragma once

#include <async_resp.hpp>
#include <dbus_utility.hpp>

namespace redfish
{

namespace chassis_utils
{

/**
 * @brief Retrieves valid chassis ID
 * @param asyncResp   Pointer to object holding response data
 * @param callback  Callback for next step to get valid chassis ID
 */
template <typename Callback>
void getValidChassisPath(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisID, Callback&& callback)
{
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    auto respHandler =
        [callback{std::forward<Callback>(callback)}, asyncResp, chassisID](
            const boost::system::error_code ec,
            const dbus::utility::MapperGetSubTreePathsResponse& chassisPaths) {
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "getValidChassisPath respHandler DBUS error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

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
                    callback(path);
                    return;
                }
            }
            callback(std::nullopt);
        };

    // Get the Chassis Collection
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

} // namespace chassis_utils
} // namespace redfish
