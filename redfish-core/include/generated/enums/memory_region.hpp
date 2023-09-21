#pragma once
#include <nlohmann/json.hpp>

namespace memory_region
{
// clang-format off

enum class RegionType{
    Invalid,
    Static,
    Dynamic,
};

NLOHMANN_JSON_SERIALIZE_ENUM(RegionType, {
    {RegionType::Invalid, "Invalid"},
    {RegionType::Static, "Static"},
    {RegionType::Dynamic, "Dynamic"},
});

}
// clang-format on
