#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(LicenseType,

    Invalid,
    Production,
    Prototype,
    Trial,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AuthorizationScope, {
    {AuthorizationScope::Invalid, "Invalid"},
    {AuthorizationScope::Device, "Device"},
    {AuthorizationScope::Capacity, "Capacity"},
    {AuthorizationScope::Service, "Service"},
});

BOOST_DESCRIBE_ENUM(AuthorizationScope,

    Invalid,
    Device,
    Capacity,
    Service,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LicenseOrigin, {
    {LicenseOrigin::Invalid, "Invalid"},
    {LicenseOrigin::BuiltIn, "BuiltIn"},
    {LicenseOrigin::Installed, "Installed"},
});

BOOST_DESCRIBE_ENUM(LicenseOrigin,

    Invalid,
    BuiltIn,
    Installed,
);

}
// clang-format on
