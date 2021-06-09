#ifndef SECUREBOOTDATABASE_V1
#define SECUREBOOTDATABASE_V1

#include "CertificateCollection_v1.h"
#include "Resource_v1.h"
#include "SecureBootDatabase_v1.h"
#include "SignatureCollection_v1.h"

enum class SecureBootDatabase_v1_ResetKeysType
{
    ResetAllKeysToDefault,
    DeleteAllKeys,
};
struct SecureBootDatabase_v1_Actions
{
    SecureBootDatabase_v1_OemActions oem;
};
struct SecureBootDatabase_v1_OemActions
{};
struct SecureBootDatabase_v1_SecureBootDatabase
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string databaseId;
    SecureBootDatabase_v1_Actions actions;
    CertificateCollection_v1_CertificateCollection certificates;
    SignatureCollection_v1_SignatureCollection signatures;
};
#endif
