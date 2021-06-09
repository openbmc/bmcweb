#ifndef SECUREBOOT_V1
#define SECUREBOOT_V1

#include "Resource_v1.h"
#include "SecureBoot_v1.h"
#include "SecureBootDatabaseCollection_v1.h"

enum class SecureBoot_v1_ResetKeysType {
    ResetAllKeysToDefault,
    DeleteAllKeys,
    DeletePK,
};
enum class SecureBoot_v1_SecureBootCurrentBootType {
    Enabled,
    Disabled,
};
enum class SecureBoot_v1_SecureBootModeType {
    SetupMode,
    UserMode,
    AuditMode,
    DeployedMode,
};
struct SecureBoot_v1_Actions
{
    SecureBoot_v1_OemActions oem;
};
struct SecureBoot_v1_OemActions
{
};
struct SecureBoot_v1_SecureBoot
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool secureBootEnable;
    SecureBoot_v1_SecureBootCurrentBootType secureBootCurrentBoot;
    SecureBoot_v1_SecureBootModeType secureBootMode;
    SecureBoot_v1_Actions actions;
    SecureBootDatabaseCollection_v1_SecureBootDatabaseCollection secureBootDatabases;
};
#endif
