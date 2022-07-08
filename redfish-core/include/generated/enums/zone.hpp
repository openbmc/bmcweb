#pragma once
#include <nlohmann/json.hpp>

namespace zone
{
// clang-format off

enum class ExternalAccessibility{
    Invalid,
    GloballyAccessible,
    NonZonedAccessible,
    ZoneOnly,
    NoInternalRouting,
};

enum class ZoneType{
    Invalid,
    Default,
    ZoneOfEndpoints,
    ZoneOfZones,
    ZoneOfResourceBlocks,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ExternalAccessibility, {
    {ExternalAccessibility::Invalid, "Invalid"},
    {ExternalAccessibility::GloballyAccessible, "GloballyAccessible"},
    {ExternalAccessibility::NonZonedAccessible, "NonZonedAccessible"},
    {ExternalAccessibility::ZoneOnly, "ZoneOnly"},
    {ExternalAccessibility::NoInternalRouting, "NoInternalRouting"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ZoneType, {
    {ZoneType::Invalid, "Invalid"},
    {ZoneType::Default, "Default"},
    {ZoneType::ZoneOfEndpoints, "ZoneOfEndpoints"},
    {ZoneType::ZoneOfZones, "ZoneOfZones"},
    {ZoneType::ZoneOfResourceBlocks, "ZoneOfResourceBlocks"},
});

}
// clang-format on
