#pragma once
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

NLOHMANN_JSON_SERIALIZE_ENUM(MediaType, {
    {MediaType::Invalid, "Invalid"},
    {MediaType::HDD, "HDD"},
    {MediaType::SSD, "SSD"},
    {MediaType::SMR, "SMR"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(HotspareType, {
    {HotspareType::Invalid, "Invalid"},
    {HotspareType::None, "None"},
    {HotspareType::Global, "Global"},
    {HotspareType::Chassis, "Chassis"},
    {HotspareType::Dedicated, "Dedicated"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EncryptionAbility, {
    {EncryptionAbility::Invalid, "Invalid"},
    {EncryptionAbility::None, "None"},
    {EncryptionAbility::SelfEncryptingDrive, "SelfEncryptingDrive"},
    {EncryptionAbility::Other, "Other"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EncryptionStatus, {
    {EncryptionStatus::Invalid, "Invalid"},
    {EncryptionStatus::Unecrypted, "Unecrypted"},
    {EncryptionStatus::Unlocked, "Unlocked"},
    {EncryptionStatus::Locked, "Locked"},
    {EncryptionStatus::Foreign, "Foreign"},
    {EncryptionStatus::Unencrypted, "Unencrypted"},
});

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

NLOHMANN_JSON_SERIALIZE_ENUM(HotspareReplacementModeType, {
    {HotspareReplacementModeType::Invalid, "Invalid"},
    {HotspareReplacementModeType::Revertible, "Revertible"},
    {HotspareReplacementModeType::NonRevertible, "NonRevertible"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DataSanitizationType, {
    {DataSanitizationType::Invalid, "Invalid"},
    {DataSanitizationType::BlockErase, "BlockErase"},
    {DataSanitizationType::CryptographicErase, "CryptographicErase"},
    {DataSanitizationType::Overwrite, "Overwrite"},
});

}
// clang-format on
