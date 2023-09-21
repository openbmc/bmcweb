#pragma once
#include <nlohmann/json.hpp>

namespace software_inventory
{
// clang-format off

enum class VersionScheme{
    Invalid,
    SemVer,
    DotIntegerNotation,
    OEM,
};

enum class ReleaseType{
    Invalid,
    Production,
    Prototype,
    Other,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VersionScheme, {
    {VersionScheme::Invalid, "Invalid"},
    {VersionScheme::SemVer, "SemVer"},
    {VersionScheme::DotIntegerNotation, "DotIntegerNotation"},
    {VersionScheme::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ReleaseType, {
    {ReleaseType::Invalid, "Invalid"},
    {ReleaseType::Production, "Production"},
    {ReleaseType::Prototype, "Prototype"},
    {ReleaseType::Other, "Other"},
});

}
// clang-format on
