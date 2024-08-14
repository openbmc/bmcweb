#pragma once

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{
namespace sensor_utils
{

inline std::string getSensorId(std::string_view sensorName,
                               std::string_view sensorType)
{
    std::string subNodeEscaped(sensorType);
    auto remove = std::ranges::remove(subNodeEscaped, '_');
    subNodeEscaped.erase(std::ranges::begin(remove), subNodeEscaped.end());

    subNodeEscaped += '_';
    subNodeEscaped += sensorName;

    return subNodeEscaped;
}

} // namespace sensor_utils
} // namespace redfish
