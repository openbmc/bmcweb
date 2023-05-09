#pragma once
#include <boost/describe/enum.hpp>
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
    AdministrateSystems,
    OperateSystems,
    AdministrateStorage,
    OperateStorageBackup,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PrivilegeType, {
    {PrivilegeType::Invalid, "Invalid"},
    {PrivilegeType::Login, "Login"},
    {PrivilegeType::ConfigureManager, "ConfigureManager"},
    {PrivilegeType::ConfigureUsers, "ConfigureUsers"},
    {PrivilegeType::ConfigureSelf, "ConfigureSelf"},
    {PrivilegeType::ConfigureComponents, "ConfigureComponents"},
    {PrivilegeType::NoAuth, "NoAuth"},
    {PrivilegeType::ConfigureCompositionInfrastructure, "ConfigureCompositionInfrastructure"},
    {PrivilegeType::AdministrateSystems, "AdministrateSystems"},
    {PrivilegeType::OperateSystems, "OperateSystems"},
    {PrivilegeType::AdministrateStorage, "AdministrateStorage"},
    {PrivilegeType::OperateStorageBackup, "OperateStorageBackup"},
});

BOOST_DESCRIBE_ENUM(PrivilegeType,

    Invalid,
    Login,
    ConfigureManager,
    ConfigureUsers,
    ConfigureSelf,
    ConfigureComponents,
    NoAuth,
    ConfigureCompositionInfrastructure,
    AdministrateSystems,
    OperateSystems,
    AdministrateStorage,
    OperateStorageBackup,
);

}
// clang-format on
