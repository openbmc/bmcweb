#ifndef MANAGERACCOUNT_V1
#define MANAGERACCOUNT_V1

#include <chrono>
#include "CertificateCollection_v1.h"
#include "ManagerAccount_v1.h"
#include "Resource_v1.h"
#include "Role_v1.h"

enum class ManagerAccount_v1_AccountTypes {
    Redfish,
    SNMP,
    OEM,
};
enum class ManagerAccount_v1_SNMPAuthenticationProtocols {
    None,
    HMAC_MD5,
    HMAC_SHA96,
};
enum class ManagerAccount_v1_SNMPEncryptionProtocols {
    None,
    CBC_DES,
    CFB128_AES128,
};
struct ManagerAccount_v1_Actions
{
    ManagerAccount_v1_OemActions oem;
};
struct ManagerAccount_v1_Links
{
    Resource_v1_Resource oem;
    Role_v1_Role role;
};
struct ManagerAccount_v1_ManagerAccount
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string password;
    std::string userName;
    std::string roleId;
    bool locked;
    bool enabled;
    ManagerAccount_v1_Links links;
    ManagerAccount_v1_Actions actions;
    CertificateCollection_v1_CertificateCollection certificates;
    bool passwordChangeRequired;
    ManagerAccount_v1_SNMPUserInfo SNMP;
    ManagerAccount_v1_AccountTypes accountTypes;
    std::string oEMAccountTypes;
    std::chrono::time_point passwordExpiration;
};
struct ManagerAccount_v1_OemActions
{
};
struct ManagerAccount_v1_SNMPUserInfo
{
    std::string authenticationKey;
    ManagerAccount_v1_SNMPAuthenticationProtocols authenticationProtocol;
    std::string encryptionKey;
    ManagerAccount_v1_SNMPEncryptionProtocols encryptionProtocol;
    bool authenticationKeySet;
    bool encryptionKeySet;
};
#endif
