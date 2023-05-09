#pragma once
#include <boost/describe/enum.hpp>
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

NLOHMANN_JSON_SERIALIZE_ENUM(ResetToDefaultsType, {
    {ResetToDefaultsType::Invalid, "Invalid"},
    {ResetToDefaultsType::ResetAll, "ResetAll"},
    {ResetToDefaultsType::PreserveVolumes, "PreserveVolumes"},
});

BOOST_DESCRIBE_ENUM(ResetToDefaultsType,

    Invalid,
    ResetAll,
    PreserveVolumes,
);

NLOHMANN_JSON_SERIALIZE_ENUM(HotspareActivationPolicy, {
    {HotspareActivationPolicy::Invalid, "Invalid"},
    {HotspareActivationPolicy::OnDriveFailure, "OnDriveFailure"},
    {HotspareActivationPolicy::OnDrivePredictedFailure, "OnDrivePredictedFailure"},
    {HotspareActivationPolicy::OEM, "OEM"},
});

BOOST_DESCRIBE_ENUM(HotspareActivationPolicy,

    Invalid,
    OnDriveFailure,
    OnDrivePredictedFailure,
    OEM,
);

NLOHMANN_JSON_SERIALIZE_ENUM(EncryptionMode, {
    {EncryptionMode::Invalid, "Invalid"},
    {EncryptionMode::Disabled, "Disabled"},
    {EncryptionMode::UseExternalKey, "UseExternalKey"},
    {EncryptionMode::UseLocalKey, "UseLocalKey"},
    {EncryptionMode::PasswordOnly, "PasswordOnly"},
    {EncryptionMode::PasswordWithExternalKey, "PasswordWithExternalKey"},
    {EncryptionMode::PasswordWithLocalKey, "PasswordWithLocalKey"},
});

BOOST_DESCRIBE_ENUM(EncryptionMode,

    Invalid,
    Disabled,
    UseExternalKey,
    UseLocalKey,
    PasswordOnly,
    PasswordWithExternalKey,
    PasswordWithLocalKey,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AutoVolumeCreate, {
    {AutoVolumeCreate::Invalid, "Invalid"},
    {AutoVolumeCreate::Disabled, "Disabled"},
    {AutoVolumeCreate::NonRAID, "NonRAID"},
    {AutoVolumeCreate::RAID0, "RAID0"},
    {AutoVolumeCreate::RAID1, "RAID1"},
});

BOOST_DESCRIBE_ENUM(AutoVolumeCreate,

    Invalid,
    Disabled,
    NonRAID,
    RAID0,
    RAID1,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ConfigurationLock, {
    {ConfigurationLock::Invalid, "Invalid"},
    {ConfigurationLock::Enabled, "Enabled"},
    {ConfigurationLock::Disabled, "Disabled"},
    {ConfigurationLock::Partial, "Partial"},
});

BOOST_DESCRIBE_ENUM(ConfigurationLock,

    Invalid,
    Enabled,
    Disabled,
    Partial,
);

NLOHMANN_JSON_SERIALIZE_ENUM(TargetConfigurationLockLevel, {
    {TargetConfigurationLockLevel::Invalid, "Invalid"},
    {TargetConfigurationLockLevel::Baseline, "Baseline"},
});

BOOST_DESCRIBE_ENUM(TargetConfigurationLockLevel,

    Invalid,
    Baseline,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ConfigLockOptions, {
    {ConfigLockOptions::Invalid, "Invalid"},
    {ConfigLockOptions::Unlocked, "Unlocked"},
    {ConfigLockOptions::Locked, "Locked"},
    {ConfigLockOptions::LockdownUnsupported, "LockdownUnsupported"},
    {ConfigLockOptions::CommandUnsupported, "CommandUnsupported"},
});

BOOST_DESCRIBE_ENUM(ConfigLockOptions,

    Invalid,
    Unlocked,
    Locked,
    LockdownUnsupported,
    CommandUnsupported,
);

}
// clang-format on
