#ifndef CERTIFICATE_V1
#define CERTIFICATE_V1

#include "Certificate_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

#include <chrono>

enum class CertificateV1CertificateType
{
    PEM,
    PKCS7,
};
enum class CertificateV1KeyUsage
{
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
struct CertificateV1OemActions
{};
struct CertificateV1Actions
{
    CertificateV1OemActions oem;
};
struct CertificateV1Identifier
{
    std::string commonName;
    std::string organization;
    std::string organizationalUnit;
    std::string city;
    std::string state;
    std::string country;
    std::string email;
};
struct CertificateV1Certificate
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string certificateString;
    CertificateV1CertificateType certificateType;
    CertificateV1Identifier issuer;
    CertificateV1Identifier subject;
    std::chrono::time_point<std::chrono::system_clock> validNotBefore;
    std::chrono::time_point<std::chrono::system_clock> validNotAfter;
    CertificateV1KeyUsage keyUsage;
    CertificateV1Actions actions;
    std::string uefiSignatureOwner;
    std::string serialNumber;
    std::string fingerprint;
    std::string fingerprintHashAlgorithm;
    std::string signatureAlgorithm;
};
struct CertificateV1RekeyResponse
{
    NavigationReference_ certificate;
    std::string cSRString;
};
struct CertificateV1RenewResponse
{
    NavigationReference_ certificate;
    std::string cSRString;
};
#endif
