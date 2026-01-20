// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
    PasswordOnly,
    PasswordWithExternalKey,
    PasswordWithLocalKey,
};

enum class AutoVolumeCreate{
    Invalid,
    Disabled,
    NonRAID,
    RAID0,
    RAID1,
};

enum class ConfigurationLock{
    Invalid,
    Enabled,
    Disabled,
    Partial,
};

enum class TargetConfigurationLockLevel{
    Invalid,
    Baseline,
};

enum class ConfigLockOptions{
    Invalid,
    Unlocked,
    Locked,
    LockdownUnsupported,
    CommandUnsupported,
};

enum class VolumeAssignmentPolicy{
    Invalid,
    Unassigned,
    Supervisor,
    WeightedRoundRobin,
};

enum class NMVeMinimumRequiredResetType{
    Invalid,
    NoReset,
    ControllerLevel,
    LimitedControllerLevel,
    NVMSubsystemReset,
    PowerCycle,
};

enum class NMVeUnfreezeAuthMode{
    Invalid,
    ProgrammedKey,
    PhysicalId,
};

enum class NMVePersonalityKeyAlgorithm{
    Invalid,
    HMAC_SHA384,
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
    {EncryptionMode::PasswordOnly, "PasswordOnly"},
    {EncryptionMode::PasswordWithExternalKey, "PasswordWithExternalKey"},
    {EncryptionMode::PasswordWithLocalKey, "PasswordWithLocalKey"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AutoVolumeCreate, {
    {AutoVolumeCreate::Invalid, "Invalid"},
    {AutoVolumeCreate::Disabled, "Disabled"},
    {AutoVolumeCreate::NonRAID, "NonRAID"},
    {AutoVolumeCreate::RAID0, "RAID0"},
    {AutoVolumeCreate::RAID1, "RAID1"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ConfigurationLock, {
    {ConfigurationLock::Invalid, "Invalid"},
    {ConfigurationLock::Enabled, "Enabled"},
    {ConfigurationLock::Disabled, "Disabled"},
    {ConfigurationLock::Partial, "Partial"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TargetConfigurationLockLevel, {
    {TargetConfigurationLockLevel::Invalid, "Invalid"},
    {TargetConfigurationLockLevel::Baseline, "Baseline"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ConfigLockOptions, {
    {ConfigLockOptions::Invalid, "Invalid"},
    {ConfigLockOptions::Unlocked, "Unlocked"},
    {ConfigLockOptions::Locked, "Locked"},
    {ConfigLockOptions::LockdownUnsupported, "LockdownUnsupported"},
    {ConfigLockOptions::CommandUnsupported, "CommandUnsupported"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(VolumeAssignmentPolicy, {
    {VolumeAssignmentPolicy::Invalid, "Invalid"},
    {VolumeAssignmentPolicy::Unassigned, "Unassigned"},
    {VolumeAssignmentPolicy::Supervisor, "Supervisor"},
    {VolumeAssignmentPolicy::WeightedRoundRobin, "WeightedRoundRobin"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NMVeMinimumRequiredResetType, {
    {NMVeMinimumRequiredResetType::Invalid, "Invalid"},
    {NMVeMinimumRequiredResetType::NoReset, "NoReset"},
    {NMVeMinimumRequiredResetType::ControllerLevel, "ControllerLevel"},
    {NMVeMinimumRequiredResetType::LimitedControllerLevel, "LimitedControllerLevel"},
    {NMVeMinimumRequiredResetType::NVMSubsystemReset, "NVMSubsystemReset"},
    {NMVeMinimumRequiredResetType::PowerCycle, "PowerCycle"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NMVeUnfreezeAuthMode, {
    {NMVeUnfreezeAuthMode::Invalid, "Invalid"},
    {NMVeUnfreezeAuthMode::ProgrammedKey, "ProgrammedKey"},
    {NMVeUnfreezeAuthMode::PhysicalId, "PhysicalId"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NMVePersonalityKeyAlgorithm, {
    {NMVePersonalityKeyAlgorithm::Invalid, "Invalid"},
    {NMVePersonalityKeyAlgorithm::HMAC_SHA384, "HMAC_SHA384"},
});

}
// clang-format on
