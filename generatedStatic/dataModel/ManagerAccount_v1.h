#ifndef MANAGERACCOUNT_V1
#define MANAGERACCOUNT_V1

#include "CertificateCollection_v1.h"
#include "ManagerAccount_v1.h"
#include "Resource_v1.h"
#include "Role_v1.h"

#include <chrono>

enum class ManagerAccountV1AccountTypes
{
    Redfish,
    SNMP,
    OEM,
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    VirtualMedia,
    WebUI,
};
enum class ManagerAccountV1SNMPAuthenticationProtocols
{
    None,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};
enum class ManagerAccountV1SNMPEncryptionProtocols
{
    None,
    CBC_DES,
    CFB128_AES128,
};
struct ManagerAccountV1OemActions
{};
struct ManagerAccountV1Actions
{
    ManagerAccountV1OemActions oem;
};
struct ManagerAccountV1Links
{
    ResourceV1Resource oem;
    RoleV1Role role;
};
struct ManagerAccountV1SNMPUserInfo
{
    std::string authenticationKey;
    ManagerAccountV1SNMPAuthenticationProtocols authenticationProtocol;
    std::string encryptionKey;
    ManagerAccountV1SNMPEncryptionProtocols encryptionProtocol;
    bool authenticationKeySet;
    bool encryptionKeySet;
};
struct ManagerAccountV1ManagerAccount
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string password;
    std::string userName;
    std::string roleId;
    bool locked;
    bool enabled;
    ManagerAccountV1Links links;
    ManagerAccountV1Actions actions;
    CertificateCollectionV1CertificateCollection certificates;
    bool passwordChangeRequired;
    ManagerAccountV1SNMPUserInfo SNMP;
    ManagerAccountV1AccountTypes accountTypes;
    std::string oEMAccountTypes;
    std::chrono::time_point<std::chrono::system_clock> passwordExpiration;
    bool strictAccountTypes;
    std::chrono::time_point<std::chrono::system_clock> accountExpiration;
    bool hostBootstrapAccount;
};
#endif
