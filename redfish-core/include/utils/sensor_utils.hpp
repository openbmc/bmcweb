#pragma once

#include <algorithm>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{
namespace sensor_utils
{

inline std::string getSensorId(std::string_view sensorName,
                               std::string_view sensorType)
{
    std::string normalizedType(sensorType);
    auto remove = std::ranges::remove(normalizedType, '_');
    normalizedType.erase(std::ranges::begin(remove), normalizedType.end());

    return std::format("{}_{}", normalizedType, sensorName);
}

inline std::pair<std::string, std::string>
    splitSensorNameAndType(std::string_view sensorId)
{
    size_t index = sensorId.find('_');
    if (index == std::string::npos)
    {
        return std::make_pair<std::string, std::string>("", "");
    }
    std::string sensorType{sensorId.substr(0, index)};
    std::string sensorName{sensorId.substr(index + 1)};
    // fan_pwm and fan_tach need special handling
    if (sensorType == "fantach" || sensorType == "fanpwm")
    {
        sensorType.insert(3, 1, '_');
    }
    return std::make_pair(sensorType, sensorName);
}

} // namespace sensor_utils
} // namespace redfish
