#pragma once
#include <nlohmann/json.hpp>

namespace license
{
// clang-format off

enum class LicenseType{
    Invalid,
    Production,
    Prototype,
    Trial,
};

enum class AuthorizationScope{
    Invalid,
    Device,
    Capacity,
    Service,
};

enum class LicenseOrigin{
    Invalid,
    BuiltIn,
    Installed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LicenseType, {
    {LicenseType::Invalid, "Invalid"},
    {LicenseType::Production, "Production"},
    {LicenseType::Prototype, "Prototype"},
    {LicenseType::Trial, "Trial"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AuthorizationScope, {
    {AuthorizationScope::Invalid, "Invalid"},
    {AuthorizationScope::Device, "Device"},
    {AuthorizationScope::Capacity, "Capacity"},
    {AuthorizationScope::Service, "Service"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LicenseOrigin, {
    {LicenseOrigin::Invalid, "Invalid"},
    {LicenseOrigin::BuiltIn, "BuiltIn"},
    {LicenseOrigin::Installed, "Installed"},
});

}
// clang-format on
