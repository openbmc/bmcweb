#pragma once
#include <nlohmann/json.hpp>

namespace leak_detector
{
// clang-format off

enum class LeakDetectorType{
    Invalid,
    Moisture,
    FloatSwitch,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LeakDetectorType, {
    {LeakDetectorType::Invalid, "Invalid"},
    {LeakDetectorType::Moisture, "Moisture"},
    {LeakDetectorType::FloatSwitch, "FloatSwitch"},
});

}
// clang-format on
