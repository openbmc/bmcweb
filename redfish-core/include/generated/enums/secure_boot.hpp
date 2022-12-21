#pragma once
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

NLOHMANN_JSON_SERIALIZE_ENUM(SecureBootModeType, {
    {SecureBootModeType::Invalid, "Invalid"},
    {SecureBootModeType::SetupMode, "SetupMode"},
    {SecureBootModeType::UserMode, "UserMode"},
    {SecureBootModeType::AuditMode, "AuditMode"},
    {SecureBootModeType::DeployedMode, "DeployedMode"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ResetKeysType, {
    {ResetKeysType::Invalid, "Invalid"},
    {ResetKeysType::ResetAllKeysToDefault, "ResetAllKeysToDefault"},
    {ResetKeysType::DeleteAllKeys, "DeleteAllKeys"},
    {ResetKeysType::DeletePK, "DeletePK"},
});

}
// clang-format on
