#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace component_integrity
{
// clang-format off

enum class ComponentIntegrityType{
    Invalid,
    SPDM,
    TPM,
    TCM,
    TPCM,
    OEM,
};

enum class MeasurementSpecification{
    Invalid,
    DMTF,
};

enum class SPDMmeasurementSummaryType{
    Invalid,
    TCB,
    All,
};

enum class DMTFmeasurementTypes{
    Invalid,
    ImmutableROM,
    MutableFirmware,
    HardwareConfiguration,
    FirmwareConfiguration,
    MutableFirmwareVersion,
    MutableFirmwareSecurityVersionNumber,
    MeasurementManifest,
};

enum class VerificationStatus{
    Invalid,
    Success,
    Failed,
};

enum class SecureSessionType{
    Invalid,
    Plain,
    EncryptedAuthenticated,
    AuthenticatedOnly,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComponentIntegrityType, {
    {ComponentIntegrityType::Invalid, "Invalid"},
    {ComponentIntegrityType::SPDM, "SPDM"},
    {ComponentIntegrityType::TPM, "TPM"},
    {ComponentIntegrityType::TCM, "TCM"},
    {ComponentIntegrityType::TPCM, "TPCM"},
    {ComponentIntegrityType::OEM, "OEM"},
});

BOOST_DESCRIBE_ENUM(ComponentIntegrityType,

    Invalid,
    SPDM,
    TPM,
    TCM,
    TPCM,
    OEM,
);

NLOHMANN_JSON_SERIALIZE_ENUM(MeasurementSpecification, {
    {MeasurementSpecification::Invalid, "Invalid"},
    {MeasurementSpecification::DMTF, "DMTF"},
});

BOOST_DESCRIBE_ENUM(MeasurementSpecification,

    Invalid,
    DMTF,
);

NLOHMANN_JSON_SERIALIZE_ENUM(SPDMmeasurementSummaryType, {
    {SPDMmeasurementSummaryType::Invalid, "Invalid"},
    {SPDMmeasurementSummaryType::TCB, "TCB"},
    {SPDMmeasurementSummaryType::All, "All"},
});

BOOST_DESCRIBE_ENUM(SPDMmeasurementSummaryType,

    Invalid,
    TCB,
    All,
);

NLOHMANN_JSON_SERIALIZE_ENUM(DMTFmeasurementTypes, {
    {DMTFmeasurementTypes::Invalid, "Invalid"},
    {DMTFmeasurementTypes::ImmutableROM, "ImmutableROM"},
    {DMTFmeasurementTypes::MutableFirmware, "MutableFirmware"},
    {DMTFmeasurementTypes::HardwareConfiguration, "HardwareConfiguration"},
    {DMTFmeasurementTypes::FirmwareConfiguration, "FirmwareConfiguration"},
    {DMTFmeasurementTypes::MutableFirmwareVersion, "MutableFirmwareVersion"},
    {DMTFmeasurementTypes::MutableFirmwareSecurityVersionNumber, "MutableFirmwareSecurityVersionNumber"},
    {DMTFmeasurementTypes::MeasurementManifest, "MeasurementManifest"},
});

BOOST_DESCRIBE_ENUM(DMTFmeasurementTypes,

    Invalid,
    ImmutableROM,
    MutableFirmware,
    HardwareConfiguration,
    FirmwareConfiguration,
    MutableFirmwareVersion,
    MutableFirmwareSecurityVersionNumber,
    MeasurementManifest,
);

NLOHMANN_JSON_SERIALIZE_ENUM(VerificationStatus, {
    {VerificationStatus::Invalid, "Invalid"},
    {VerificationStatus::Success, "Success"},
    {VerificationStatus::Failed, "Failed"},
});

BOOST_DESCRIBE_ENUM(VerificationStatus,

    Invalid,
    Success,
    Failed,
);

NLOHMANN_JSON_SERIALIZE_ENUM(SecureSessionType, {
    {SecureSessionType::Invalid, "Invalid"},
    {SecureSessionType::Plain, "Plain"},
    {SecureSessionType::EncryptedAuthenticated, "EncryptedAuthenticated"},
    {SecureSessionType::AuthenticatedOnly, "AuthenticatedOnly"},
});

BOOST_DESCRIBE_ENUM(SecureSessionType,

    Invalid,
    Plain,
    EncryptedAuthenticated,
    AuthenticatedOnly,
);

}
// clang-format on
