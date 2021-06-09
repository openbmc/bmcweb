#ifndef SECUREBOOT_V1
#define SECUREBOOT_V1

#include "Resource_v1.h"
#include "SecureBootDatabaseCollection_v1.h"
#include "SecureBoot_v1.h"

enum class SecureBootV1ResetKeysType
{
    ResetAllKeysToDefault,
    DeleteAllKeys,
    DeletePK,
};
enum class SecureBootV1SecureBootCurrentBootType
{
    Enabled,
    Disabled,
};
enum class SecureBootV1SecureBootModeType
{
    SetupMode,
    UserMode,
    AuditMode,
    DeployedMode,
};
struct SecureBootV1OemActions
{};
struct SecureBootV1Actions
{
    SecureBootV1OemActions oem;
};
struct SecureBootV1SecureBoot
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool secureBootEnable;
    SecureBootV1SecureBootCurrentBootType secureBootCurrentBoot;
    SecureBootV1SecureBootModeType secureBootMode;
    SecureBootV1Actions actions;
    SecureBootDatabaseCollectionV1SecureBootDatabaseCollection
        secureBootDatabases;
};
#endif
