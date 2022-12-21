#pragma once
#include <nlohmann/json.hpp>

namespace component_integrity
{
// clang-format off

enum class ComponentIntegrityType{
    Invalid,
    SPDM,
    TPM,
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
    {ComponentIntegrityType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MeasurementSpecification, {
    {MeasurementSpecification::Invalid, "Invalid"},
    {MeasurementSpecification::DMTF, "DMTF"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SPDMmeasurementSummaryType, {
    {SPDMmeasurementSummaryType::Invalid, "Invalid"},
    {SPDMmeasurementSummaryType::TCB, "TCB"},
    {SPDMmeasurementSummaryType::All, "All"},
});

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

NLOHMANN_JSON_SERIALIZE_ENUM(VerificationStatus, {
    {VerificationStatus::Invalid, "Invalid"},
    {VerificationStatus::Success, "Success"},
    {VerificationStatus::Failed, "Failed"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SecureSessionType, {
    {SecureSessionType::Invalid, "Invalid"},
    {SecureSessionType::Plain, "Plain"},
    {SecureSessionType::EncryptedAuthenticated, "EncryptedAuthenticated"},
    {SecureSessionType::AuthenticatedOnly, "AuthenticatedOnly"},
});

}
// clang-format on
