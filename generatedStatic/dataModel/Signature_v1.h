#ifndef SIGNATURE_V1
#define SIGNATURE_V1

#include "Resource_v1.h"
#include "Signature_v1.h"

enum class SignatureV1SignatureTypeRegistry
{
    UEFI,
};
struct SignatureV1OemActions
{};
struct SignatureV1Actions
{
    SignatureV1OemActions oem;
};
struct SignatureV1Signature
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    SignatureV1SignatureTypeRegistry signatureTypeRegistry;
    std::string signatureType;
    std::string signatureString;
    std::string uefiSignatureOwner;
    SignatureV1Actions actions;
};
#endif
