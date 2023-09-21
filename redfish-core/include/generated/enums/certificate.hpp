#pragma once
#include <nlohmann/json.hpp>

namespace certificate
{
// clang-format off

enum class CertificateType{
    Invalid,
    PEM,
    PEMchain,
    PKCS7,
};

enum class KeyUsage{
    Invalid,
    DigitalSignature,
    NonRepudiation,
    KeyEncipherment,
    DataEncipherment,
    KeyAgreement,
    KeyCertSign,
    CRLSigning,
    EncipherOnly,
    DecipherOnly,
    ServerAuthentication,
    ClientAuthentication,
    CodeSigning,
    EmailProtection,
    Timestamping,
    OCSPSigning,
};

enum class CertificateUsageType{
    Invalid,
    User,
    Web,
    SSH,
    Device,
    Platform,
    BIOS,
    IDevID,
    LDevID,
    IAK,
    LAK,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CertificateType, {
    {CertificateType::Invalid, "Invalid"},
    {CertificateType::PEM, "PEM"},
    {CertificateType::PEMchain, "PEMchain"},
    {CertificateType::PKCS7, "PKCS7"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(KeyUsage, {
    {KeyUsage::Invalid, "Invalid"},
    {KeyUsage::DigitalSignature, "DigitalSignature"},
    {KeyUsage::NonRepudiation, "NonRepudiation"},
    {KeyUsage::KeyEncipherment, "KeyEncipherment"},
    {KeyUsage::DataEncipherment, "DataEncipherment"},
    {KeyUsage::KeyAgreement, "KeyAgreement"},
    {KeyUsage::KeyCertSign, "KeyCertSign"},
    {KeyUsage::CRLSigning, "CRLSigning"},
    {KeyUsage::EncipherOnly, "EncipherOnly"},
    {KeyUsage::DecipherOnly, "DecipherOnly"},
    {KeyUsage::ServerAuthentication, "ServerAuthentication"},
    {KeyUsage::ClientAuthentication, "ClientAuthentication"},
    {KeyUsage::CodeSigning, "CodeSigning"},
    {KeyUsage::EmailProtection, "EmailProtection"},
    {KeyUsage::Timestamping, "Timestamping"},
    {KeyUsage::OCSPSigning, "OCSPSigning"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CertificateUsageType, {
    {CertificateUsageType::Invalid, "Invalid"},
    {CertificateUsageType::User, "User"},
    {CertificateUsageType::Web, "Web"},
    {CertificateUsageType::SSH, "SSH"},
    {CertificateUsageType::Device, "Device"},
    {CertificateUsageType::Platform, "Platform"},
    {CertificateUsageType::BIOS, "BIOS"},
    {CertificateUsageType::IDevID, "IDevID"},
    {CertificateUsageType::LDevID, "LDevID"},
    {CertificateUsageType::IAK, "IAK"},
    {CertificateUsageType::LAK, "LAK"},
});

}
// clang-format on
