#ifndef SIGNATURE_V1
#define SIGNATURE_V1

#include "Resource_v1.h"
#include "Signature_v1.h"

enum class Signature_v1_SignatureTypeRegistry
{
    UEFI,
};
struct Signature_v1_Actions
{
    Signature_v1_OemActions oem;
};
struct Signature_v1_OemActions
{};
struct Signature_v1_Signature
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Signature_v1_SignatureTypeRegistry signatureTypeRegistry;
    std::string signatureType;
    std::string signatureString;
    string uefiSignatureOwner;
    Signature_v1_Actions actions;
};
#endif
