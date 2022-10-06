#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace redfish
{
namespace fan_utils
{
constexpr std::array<std::string_view, 1> fanInterface = {
    "xyz.openbmc_project.Inventory.Item.Fan"};
constexpr std::array<std::string_view, 1> sensorInterface = {
    "xyz.openbmc_project.Sensor.Value"};

inline void afterGetFanSensorObjects(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::function<
        void(std::vector<std::pair<std::string, std::string>>&)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error for getAssociatedSubTree {}",
                             ec.value());
            messages::internalError(asyncResp->res);
        }
        return;
    }

    std::vector<std::pair<std::string, std::string>> sensorsPathAndService;

    for (const auto& [sensorPath, serviceMaps] : subtree)
    {
        for (const auto& [service, interfaces] : serviceMaps)
        {
            sensorsPathAndService.emplace_back(service, sensorPath);
        }
    }

    /* Sort list by sensorPath */
    std::sort(sensorsPathAndService.begin(), sensorsPathAndService.end(),
              [](const std::pair<std::string, std::string>& c1,
                 const std::pair<std::string, std::string>& c2) {
        return c1.second < c2.second;
    });

    callback(sensorsPathAndService);
}

inline void getFanSensorObjects(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& fanPath,
    const std::function<
        void(std::vector<std::pair<std::string, std::string>>&)>& callback)
{
    sdbusplus::message::object_path endpointPath{fanPath};
    endpointPath /= "all_sensors";

    dbus::utility::getAssociatedSubTree(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/sensors"), 0,
        sensorInterface,
        std::bind_front(afterGetFanSensorObjects, asyncResp, callback));
}

inline void getFanPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<void(const dbus::utility::MapperGetSubTreePathsResponse&
                                 fanPaths)>& callback)
{
    sdbusplus::message::object_path endpointPath{validChassisPath};
    endpointPath /= "cooled_by";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/inventory"), 0,
        fanInterface,
        [asyncResp, callback](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& subtreePaths) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR(
                    "DBUS response error for getAssociatedSubTreePaths {}",
                    ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }
        callback(subtreePaths);
    });
}

/**
 * @brief Gets the Id and Name for a fan sensor from its D-Bus path
 * @details On error sensorId and sensorName are empty.
 * @param[in] sensorPath  The D-Bus path to the sensor
 * @param[out] sensorId   The Redfish Id for the sensor is returned here.
 * @param[out] sensorName The Redfish Name for the sensor is returned here.
 */
inline void getFanSensorIdAndName(const std::string& sensorPath,
                                  std::string& sensorId,
                                  std::string& sensorName)
{
    sdbusplus::message::object_path path(sensorPath);
    sensorName = path.filename();
    if (sensorName.empty())
    {
        /* Let caller handle any errors */
        BMCWEB_LOG_DEBUG("Invalid sensor path {}", sensorPath);
        sensorId.clear();
        return;
    }

    std::string type = path.parent_path().filename();

    /* Validate type is for a fan */
    if (type != "fan_tach" && type != "fan_pwm")
    {
        BMCWEB_LOG_DEBUG("Not a fan sensor type {}", type);
        sensorId.clear();
        sensorName.clear();
        return;
    }

    /* Remove the underscore in type to "normalize" it in the URI */
    auto remove = std::ranges::remove(type, '_');
    type.erase(std::ranges::begin(remove), type.end());

    sensorId = type;
    sensorId += "_";
    sensorId += sensorName;

    /* Replace any underscores with blank spaces to "normalize" the name */
    std::replace(sensorName.begin(), sensorName.end(), '_', ' ');
}

} // namespace fan_utils
} // namespace redfish
