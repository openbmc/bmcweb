#ifndef CERTIFICATE_V1
#define CERTIFICATE_V1

#include "Certificate_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

#include <chrono>

enum class Certificate_v1_CertificateType
{
    PEM,
    PKCS7,
};
enum class Certificate_v1_KeyUsage
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
struct Certificate_v1_Actions
{
    Certificate_v1_OemActions oem;
};
struct Certificate_v1_Certificate
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string certificateString;
    Certificate_v1_CertificateType certificateType;
    Certificate_v1_Identifier issuer;
    Certificate_v1_Identifier subject;
    std::chrono::time_point validNotBefore;
    std::chrono::time_point validNotAfter;
    Certificate_v1_KeyUsage keyUsage;
    Certificate_v1_Actions actions;
    string uefiSignatureOwner;
};
struct Certificate_v1_Identifier
{
    std::string commonName;
    std::string organization;
    std::string organizationalUnit;
    std::string city;
    std::string state;
    std::string country;
    std::string email;
};
struct Certificate_v1_OemActions
{};
struct Certificate_v1_RekeyResponse
{
    NavigationReference__ certificate;
    std::string cSRString;
};
struct Certificate_v1_RenewResponse
{
    NavigationReference__ certificate;
    std::string cSRString;
};
#endif
