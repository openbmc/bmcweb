#pragma once
#include <nlohmann/json.hpp>

namespace privileges
{
// clang-format off

enum class PrivilegeType{
    Invalid,
    Login,
    ConfigureManager,
    ConfigureUsers,
    ConfigureSelf,
    ConfigureComponents,
    NoAuth,
    ConfigureCompositionInfrastructure,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PrivilegeType, { //NOLINT
    {PrivilegeType::Invalid, "Invalid"},
    {PrivilegeType::Login, "Login"},
    {PrivilegeType::ConfigureManager, "ConfigureManager"},
    {PrivilegeType::ConfigureUsers, "ConfigureUsers"},
    {PrivilegeType::ConfigureSelf, "ConfigureSelf"},
    {PrivilegeType::ConfigureComponents, "ConfigureComponents"},
    {PrivilegeType::NoAuth, "NoAuth"},
    {PrivilegeType::ConfigureCompositionInfrastructure, "ConfigureCompositionInfrastructure"},
});

}
// clang-format on
