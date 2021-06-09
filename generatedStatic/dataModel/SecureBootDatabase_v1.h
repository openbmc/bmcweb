#ifndef SECUREBOOTDATABASE_V1
#define SECUREBOOTDATABASE_V1

#include "CertificateCollection_v1.h"
#include "Resource_v1.h"
#include "SecureBootDatabase_v1.h"
#include "SignatureCollection_v1.h"

enum class SecureBootDatabaseV1ResetKeysType
{
    ResetAllKeysToDefault,
    DeleteAllKeys,
};
struct SecureBootDatabaseV1OemActions
{};
struct SecureBootDatabaseV1Actions
{
    SecureBootDatabaseV1OemActions oem;
};
struct SecureBootDatabaseV1SecureBootDatabase
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string databaseId;
    SecureBootDatabaseV1Actions actions;
    CertificateCollectionV1CertificateCollection certificates;
    SignatureCollectionV1SignatureCollection signatures;
};
#endif
