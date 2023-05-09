#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace drive
{
// clang-format off

enum class MediaType{
    Invalid,
    HDD,
    SSD,
    SMR,
};

enum class HotspareType{
    Invalid,
    None,
    Global,
    Chassis,
    Dedicated,
};

enum class EncryptionAbility{
    Invalid,
    None,
    SelfEncryptingDrive,
    Other,
};

enum class EncryptionStatus{
    Invalid,
    Unecrypted,
    Unlocked,
    Locked,
    Foreign,
    Unencrypted,
};

enum class StatusIndicator{
    Invalid,
    OK,
    Fail,
    Rebuild,
    PredictiveFailureAnalysis,
    Hotspare,
    InACriticalArray,
    InAFailedArray,
};

enum class HotspareReplacementModeType{
    Invalid,
    Revertible,
    NonRevertible,
};

enum class DataSanitizationType{
    Invalid,
    BlockErase,
    CryptographicErase,
    Overwrite,
};

enum class FormFactor{
    Invalid,
    Drive3_5,
    Drive2_5,
    EDSFF,
    EDSFF_1U_Long,
    EDSFF_1U_Short,
    EDSFF_E3_Short,
    EDSFF_E3_Long,
    M2,
    M2_2230,
    M2_2242,
    M2_2260,
    M2_2280,
    M2_22110,
    U2,
    PCIeSlotFullLength,
    PCIeSlotLowProfile,
    PCIeHalfLength,
    OEM,
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

NLOHMANN_JSON_SERIALIZE_ENUM(MediaType, {
    {MediaType::Invalid, "Invalid"},
    {MediaType::HDD, "HDD"},
    {MediaType::SSD, "SSD"},
    {MediaType::SMR, "SMR"},
});

BOOST_DESCRIBE_ENUM(MediaType,

    Invalid,
    HDD,
    SSD,
    SMR,
);

NLOHMANN_JSON_SERIALIZE_ENUM(HotspareType, {
    {HotspareType::Invalid, "Invalid"},
    {HotspareType::None, "None"},
    {HotspareType::Global, "Global"},
    {HotspareType::Chassis, "Chassis"},
    {HotspareType::Dedicated, "Dedicated"},
});

BOOST_DESCRIBE_ENUM(HotspareType,

    Invalid,
    None,
    Global,
    Chassis,
    Dedicated,
);

NLOHMANN_JSON_SERIALIZE_ENUM(EncryptionAbility, {
    {EncryptionAbility::Invalid, "Invalid"},
    {EncryptionAbility::None, "None"},
    {EncryptionAbility::SelfEncryptingDrive, "SelfEncryptingDrive"},
    {EncryptionAbility::Other, "Other"},
});

BOOST_DESCRIBE_ENUM(EncryptionAbility,

    Invalid,
    None,
    SelfEncryptingDrive,
    Other,
);

NLOHMANN_JSON_SERIALIZE_ENUM(EncryptionStatus, {
    {EncryptionStatus::Invalid, "Invalid"},
    {EncryptionStatus::Unecrypted, "Unecrypted"},
    {EncryptionStatus::Unlocked, "Unlocked"},
    {EncryptionStatus::Locked, "Locked"},
    {EncryptionStatus::Foreign, "Foreign"},
    {EncryptionStatus::Unencrypted, "Unencrypted"},
});

BOOST_DESCRIBE_ENUM(EncryptionStatus,

    Invalid,
    Unecrypted,
    Unlocked,
    Locked,
    Foreign,
    Unencrypted,
);

NLOHMANN_JSON_SERIALIZE_ENUM(StatusIndicator, {
    {StatusIndicator::Invalid, "Invalid"},
    {StatusIndicator::OK, "OK"},
    {StatusIndicator::Fail, "Fail"},
    {StatusIndicator::Rebuild, "Rebuild"},
    {StatusIndicator::PredictiveFailureAnalysis, "PredictiveFailureAnalysis"},
    {StatusIndicator::Hotspare, "Hotspare"},
    {StatusIndicator::InACriticalArray, "InACriticalArray"},
    {StatusIndicator::InAFailedArray, "InAFailedArray"},
});

BOOST_DESCRIBE_ENUM(StatusIndicator,

    Invalid,
    OK,
    Fail,
    Rebuild,
    PredictiveFailureAnalysis,
    Hotspare,
    InACriticalArray,
    InAFailedArray,
);

NLOHMANN_JSON_SERIALIZE_ENUM(HotspareReplacementModeType, {
    {HotspareReplacementModeType::Invalid, "Invalid"},
    {HotspareReplacementModeType::Revertible, "Revertible"},
    {HotspareReplacementModeType::NonRevertible, "NonRevertible"},
});

BOOST_DESCRIBE_ENUM(HotspareReplacementModeType,

    Invalid,
    Revertible,
    NonRevertible,
);

NLOHMANN_JSON_SERIALIZE_ENUM(DataSanitizationType, {
    {DataSanitizationType::Invalid, "Invalid"},
    {DataSanitizationType::BlockErase, "BlockErase"},
    {DataSanitizationType::CryptographicErase, "CryptographicErase"},
    {DataSanitizationType::Overwrite, "Overwrite"},
});

BOOST_DESCRIBE_ENUM(DataSanitizationType,

    Invalid,
    BlockErase,
    CryptographicErase,
    Overwrite,
);

NLOHMANN_JSON_SERIALIZE_ENUM(FormFactor, {
    {FormFactor::Invalid, "Invalid"},
    {FormFactor::Drive3_5, "Drive3_5"},
    {FormFactor::Drive2_5, "Drive2_5"},
    {FormFactor::EDSFF, "EDSFF"},
    {FormFactor::EDSFF_1U_Long, "EDSFF_1U_Long"},
    {FormFactor::EDSFF_1U_Short, "EDSFF_1U_Short"},
    {FormFactor::EDSFF_E3_Short, "EDSFF_E3_Short"},
    {FormFactor::EDSFF_E3_Long, "EDSFF_E3_Long"},
    {FormFactor::M2, "M2"},
    {FormFactor::M2_2230, "M2_2230"},
    {FormFactor::M2_2242, "M2_2242"},
    {FormFactor::M2_2260, "M2_2260"},
    {FormFactor::M2_2280, "M2_2280"},
    {FormFactor::M2_22110, "M2_22110"},
    {FormFactor::U2, "U2"},
    {FormFactor::PCIeSlotFullLength, "PCIeSlotFullLength"},
    {FormFactor::PCIeSlotLowProfile, "PCIeSlotLowProfile"},
    {FormFactor::PCIeHalfLength, "PCIeHalfLength"},
    {FormFactor::OEM, "OEM"},
});

BOOST_DESCRIBE_ENUM(FormFactor,

    Invalid,
    Drive3_5,
    Drive2_5,
    EDSFF,
    EDSFF_1U_Long,
    EDSFF_1U_Short,
    EDSFF_E3_Short,
    EDSFF_E3_Long,
    M2,
    M2_2230,
    M2_2242,
    M2_2260,
    M2_2280,
    M2_22110,
    U2,
    PCIeSlotFullLength,
    PCIeSlotLowProfile,
    PCIeHalfLength,
    OEM,
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
