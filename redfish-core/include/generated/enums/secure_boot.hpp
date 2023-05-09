#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace secure_boot
{
// clang-format off

enum class SecureBootCurrentBootType{
    Invalid,
    Enabled,
    Disabled,
};

enum class SecureBootModeType{
    Invalid,
    SetupMode,
    UserMode,
    AuditMode,
    DeployedMode,
};

enum class ResetKeysType{
    Invalid,
    ResetAllKeysToDefault,
    DeleteAllKeys,
    DeletePK,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SecureBootCurrentBootType, {
    {SecureBootCurrentBootType::Invalid, "Invalid"},
    {SecureBootCurrentBootType::Enabled, "Enabled"},
    {SecureBootCurrentBootType::Disabled, "Disabled"},
});

BOOST_DESCRIBE_ENUM(SecureBootCurrentBootType,

    Invalid,
    Enabled,
    Disabled,
);

NLOHMANN_JSON_SERIALIZE_ENUM(SecureBootModeType, {
    {SecureBootModeType::Invalid, "Invalid"},
    {SecureBootModeType::SetupMode, "SetupMode"},
    {SecureBootModeType::UserMode, "UserMode"},
    {SecureBootModeType::AuditMode, "AuditMode"},
    {SecureBootModeType::DeployedMode, "DeployedMode"},
});

BOOST_DESCRIBE_ENUM(SecureBootModeType,

    Invalid,
    SetupMode,
    UserMode,
    AuditMode,
    DeployedMode,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ResetKeysType, {
    {ResetKeysType::Invalid, "Invalid"},
    {ResetKeysType::ResetAllKeysToDefault, "ResetAllKeysToDefault"},
    {ResetKeysType::DeleteAllKeys, "DeleteAllKeys"},
    {ResetKeysType::DeletePK, "DeletePK"},
});

BOOST_DESCRIBE_ENUM(ResetKeysType,

    Invalid,
    ResetAllKeysToDefault,
    DeleteAllKeys,
    DeletePK,
);

}
// clang-format on
