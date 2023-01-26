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

NLOHMANN_JSON_SERIALIZE_ENUM(VersionScheme, {
    {VersionScheme::Invalid, "Invalid"},
    {VersionScheme::SemVer, "SemVer"},
    {VersionScheme::DotIntegerNotation, "DotIntegerNotation"},
    {VersionScheme::OEM, "OEM"},
});

}
// clang-format on
