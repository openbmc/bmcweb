#pragma once
#include <nlohmann/json.hpp>

namespace storage
{
// clang-format off

enum class ResetToDefaultsType{
    Invalid,
    ResetAll,
    PreserveVolumes,
};

enum class HotspareActivationPolicy{
    Invalid,
    OnDriveFailure,
    OnDrivePredictedFailure,
    OEM,
};

enum class EncryptionMode{
    Invalid,
    Disabled,
    UseExternalKey,
    UseLocalKey,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResetToDefaultsType, {
    {ResetToDefaultsType::Invalid, "Invalid"},
    {ResetToDefaultsType::ResetAll, "ResetAll"},
    {ResetToDefaultsType::PreserveVolumes, "PreserveVolumes"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(HotspareActivationPolicy, {
    {HotspareActivationPolicy::Invalid, "Invalid"},
    {HotspareActivationPolicy::OnDriveFailure, "OnDriveFailure"},
    {HotspareActivationPolicy::OnDrivePredictedFailure, "OnDrivePredictedFailure"},
    {HotspareActivationPolicy::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EncryptionMode, {
    {EncryptionMode::Invalid, "Invalid"},
    {EncryptionMode::Disabled, "Disabled"},
    {EncryptionMode::UseExternalKey, "UseExternalKey"},
    {EncryptionMode::UseLocalKey, "UseLocalKey"},
});

}
// clang-format on
