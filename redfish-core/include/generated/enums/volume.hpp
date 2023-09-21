#pragma once
#include <nlohmann/json.hpp>

namespace volume
{
// clang-format off

enum class InitializeType{
    Invalid,
    Fast,
    Slow,
};

enum class InitializeMethod{
    Invalid,
    Skip,
    Background,
    Foreground,
};

enum class RAIDType{
    Invalid,
    RAID0,
    RAID1,
    RAID3,
    RAID4,
    RAID5,
    RAID6,
    RAID10,
    RAID01,
    RAID6TP,
    RAID1E,
    RAID50,
    RAID60,
    RAID00,
    RAID10E,
    RAID1Triple,
    RAID10Triple,
    None,
};

enum class VolumeType{
    Invalid,
    RawDevice,
    NonRedundant,
    Mirrored,
    StripedWithParity,
    SpannedMirrors,
    SpannedStripesWithParity,
};

enum class EncryptionTypes{
    Invalid,
    NativeDriveEncryption,
    ControllerAssisted,
    SoftwareAssisted,
};

enum class WriteHoleProtectionPolicyType{
    Invalid,
    Off,
    Journaling,
    DistributedLog,
    Oem,
};

enum class VolumeUsageType{
    Invalid,
    Data,
    SystemData,
    CacheOnly,
    SystemReserve,
    ReplicationReserve,
};

enum class ReadCachePolicyType{
    Invalid,
    ReadAhead,
    AdaptiveReadAhead,
    Off,
};

enum class WriteCachePolicyType{
    Invalid,
    WriteThrough,
    ProtectedWriteBack,
    UnprotectedWriteBack,
    Off,
};

enum class WriteCacheStateType{
    Invalid,
    Unprotected,
    Protected,
    Degraded,
};

enum class LBAFormatType{
    Invalid,
    LBAFormat0,
    LBAFormat1,
    LBAFormat2,
    LBAFormat3,
    LBAFormat4,
    LBAFormat5,
    LBAFormat6,
    LBAFormat7,
    LBAFormat8,
    LBAFormat9,
    LBAFormat10,
    LBAFormat11,
    LBAFormat12,
    LBAFormat13,
    LBAFormat14,
    LBAFormat15,
};

enum class NamespaceType{
    Invalid,
    Block,
    KeyValue,
    ZNS,
    Computational,
};

enum class OperationType{
    Invalid,
    Deduplicate,
    CheckConsistency,
    Initialize,
    Replicate,
    Delete,
    ChangeRAIDType,
    Rebuild,
    Encrypt,
    Decrypt,
    Resize,
    Compress,
    Sanitize,
    Format,
    ChangeStripSize,
};

enum class LBARelativePerformanceType{
    Invalid,
    Best,
    Better,
    Good,
    Degraded,
};

NLOHMANN_JSON_SERIALIZE_ENUM(InitializeType, {
    {InitializeType::Invalid, "Invalid"},
    {InitializeType::Fast, "Fast"},
    {InitializeType::Slow, "Slow"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(InitializeMethod, {
    {InitializeMethod::Invalid, "Invalid"},
    {InitializeMethod::Skip, "Skip"},
    {InitializeMethod::Background, "Background"},
    {InitializeMethod::Foreground, "Foreground"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(RAIDType, {
    {RAIDType::Invalid, "Invalid"},
    {RAIDType::RAID0, "RAID0"},
    {RAIDType::RAID1, "RAID1"},
    {RAIDType::RAID3, "RAID3"},
    {RAIDType::RAID4, "RAID4"},
    {RAIDType::RAID5, "RAID5"},
    {RAIDType::RAID6, "RAID6"},
    {RAIDType::RAID10, "RAID10"},
    {RAIDType::RAID01, "RAID01"},
    {RAIDType::RAID6TP, "RAID6TP"},
    {RAIDType::RAID1E, "RAID1E"},
    {RAIDType::RAID50, "RAID50"},
    {RAIDType::RAID60, "RAID60"},
    {RAIDType::RAID00, "RAID00"},
    {RAIDType::RAID10E, "RAID10E"},
    {RAIDType::RAID1Triple, "RAID1Triple"},
    {RAIDType::RAID10Triple, "RAID10Triple"},
    {RAIDType::None, "None"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(VolumeType, {
    {VolumeType::Invalid, "Invalid"},
    {VolumeType::RawDevice, "RawDevice"},
    {VolumeType::NonRedundant, "NonRedundant"},
    {VolumeType::Mirrored, "Mirrored"},
    {VolumeType::StripedWithParity, "StripedWithParity"},
    {VolumeType::SpannedMirrors, "SpannedMirrors"},
    {VolumeType::SpannedStripesWithParity, "SpannedStripesWithParity"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(EncryptionTypes, {
    {EncryptionTypes::Invalid, "Invalid"},
    {EncryptionTypes::NativeDriveEncryption, "NativeDriveEncryption"},
    {EncryptionTypes::ControllerAssisted, "ControllerAssisted"},
    {EncryptionTypes::SoftwareAssisted, "SoftwareAssisted"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WriteHoleProtectionPolicyType, {
    {WriteHoleProtectionPolicyType::Invalid, "Invalid"},
    {WriteHoleProtectionPolicyType::Off, "Off"},
    {WriteHoleProtectionPolicyType::Journaling, "Journaling"},
    {WriteHoleProtectionPolicyType::DistributedLog, "DistributedLog"},
    {WriteHoleProtectionPolicyType::Oem, "Oem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(VolumeUsageType, {
    {VolumeUsageType::Invalid, "Invalid"},
    {VolumeUsageType::Data, "Data"},
    {VolumeUsageType::SystemData, "SystemData"},
    {VolumeUsageType::CacheOnly, "CacheOnly"},
    {VolumeUsageType::SystemReserve, "SystemReserve"},
    {VolumeUsageType::ReplicationReserve, "ReplicationReserve"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ReadCachePolicyType, {
    {ReadCachePolicyType::Invalid, "Invalid"},
    {ReadCachePolicyType::ReadAhead, "ReadAhead"},
    {ReadCachePolicyType::AdaptiveReadAhead, "AdaptiveReadAhead"},
    {ReadCachePolicyType::Off, "Off"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WriteCachePolicyType, {
    {WriteCachePolicyType::Invalid, "Invalid"},
    {WriteCachePolicyType::WriteThrough, "WriteThrough"},
    {WriteCachePolicyType::ProtectedWriteBack, "ProtectedWriteBack"},
    {WriteCachePolicyType::UnprotectedWriteBack, "UnprotectedWriteBack"},
    {WriteCachePolicyType::Off, "Off"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WriteCacheStateType, {
    {WriteCacheStateType::Invalid, "Invalid"},
    {WriteCacheStateType::Unprotected, "Unprotected"},
    {WriteCacheStateType::Protected, "Protected"},
    {WriteCacheStateType::Degraded, "Degraded"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LBAFormatType, {
    {LBAFormatType::Invalid, "Invalid"},
    {LBAFormatType::LBAFormat0, "LBAFormat0"},
    {LBAFormatType::LBAFormat1, "LBAFormat1"},
    {LBAFormatType::LBAFormat2, "LBAFormat2"},
    {LBAFormatType::LBAFormat3, "LBAFormat3"},
    {LBAFormatType::LBAFormat4, "LBAFormat4"},
    {LBAFormatType::LBAFormat5, "LBAFormat5"},
    {LBAFormatType::LBAFormat6, "LBAFormat6"},
    {LBAFormatType::LBAFormat7, "LBAFormat7"},
    {LBAFormatType::LBAFormat8, "LBAFormat8"},
    {LBAFormatType::LBAFormat9, "LBAFormat9"},
    {LBAFormatType::LBAFormat10, "LBAFormat10"},
    {LBAFormatType::LBAFormat11, "LBAFormat11"},
    {LBAFormatType::LBAFormat12, "LBAFormat12"},
    {LBAFormatType::LBAFormat13, "LBAFormat13"},
    {LBAFormatType::LBAFormat14, "LBAFormat14"},
    {LBAFormatType::LBAFormat15, "LBAFormat15"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NamespaceType, {
    {NamespaceType::Invalid, "Invalid"},
    {NamespaceType::Block, "Block"},
    {NamespaceType::KeyValue, "KeyValue"},
    {NamespaceType::ZNS, "ZNS"},
    {NamespaceType::Computational, "Computational"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(OperationType, {
    {OperationType::Invalid, "Invalid"},
    {OperationType::Deduplicate, "Deduplicate"},
    {OperationType::CheckConsistency, "CheckConsistency"},
    {OperationType::Initialize, "Initialize"},
    {OperationType::Replicate, "Replicate"},
    {OperationType::Delete, "Delete"},
    {OperationType::ChangeRAIDType, "ChangeRAIDType"},
    {OperationType::Rebuild, "Rebuild"},
    {OperationType::Encrypt, "Encrypt"},
    {OperationType::Decrypt, "Decrypt"},
    {OperationType::Resize, "Resize"},
    {OperationType::Compress, "Compress"},
    {OperationType::Sanitize, "Sanitize"},
    {OperationType::Format, "Format"},
    {OperationType::ChangeStripSize, "ChangeStripSize"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LBARelativePerformanceType, {
    {LBARelativePerformanceType::Invalid, "Invalid"},
    {LBARelativePerformanceType::Best, "Best"},
    {LBARelativePerformanceType::Better, "Better"},
    {LBARelativePerformanceType::Good, "Good"},
    {LBARelativePerformanceType::Degraded, "Degraded"},
});

}
// clang-format on
