#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(LeakDetectorType,

    Invalid,
    Moisture,
    FloatSwitch,
);

}
// clang-format on
