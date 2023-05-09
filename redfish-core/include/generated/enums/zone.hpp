#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(ExternalAccessibility,

    Invalid,
    GloballyAccessible,
    NonZonedAccessible,
    ZoneOnly,
    NoInternalRouting,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ZoneType, {
    {ZoneType::Invalid, "Invalid"},
    {ZoneType::Default, "Default"},
    {ZoneType::ZoneOfEndpoints, "ZoneOfEndpoints"},
    {ZoneType::ZoneOfZones, "ZoneOfZones"},
    {ZoneType::ZoneOfResourceBlocks, "ZoneOfResourceBlocks"},
});

BOOST_DESCRIBE_ENUM(ZoneType,

    Invalid,
    Default,
    ZoneOfEndpoints,
    ZoneOfZones,
    ZoneOfResourceBlocks,
);

}
// clang-format on
