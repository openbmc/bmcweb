#ifndef ACCOUNTSERVICE_V1
#define ACCOUNTSERVICE_V1

#include "AccountService_v1.h"
#include "CertificateCollection_v1.h"
#include "ExternalAccountProviderCollection_v1.h"
#include "ManagerAccountCollection_v1.h"
#include "ManagerAccount_v1.h"
#include "PrivilegeRegistry_v1.h"
#include "Privileges_v1.h"
#include "Resource_v1.h"
#include "RoleCollection_v1.h"

enum class AccountServiceV1AccountProviderTypes
{
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
    TACACSplus,
};
enum class AccountServiceV1AuthenticationTypes
{
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
};
enum class AccountServiceV1LocalAccountAuth
{
    Enabled,
    Disabled,
    Fallback,
    LocalFirst,
};
enum class AccountServiceV1TACACSplusPasswordExchangeProtocol
{
    ASCII,
    PAP,
    CHAP,
    MSCHAPv1,
    MSCHAPv2,
};
struct AccountServiceV1OemActions
{};
struct AccountServiceV1Actions
{
    AccountServiceV1OemActions oem;
};
struct AccountServiceV1Authentication
{
    AccountServiceV1AuthenticationTypes authenticationType;
    std::string username;
    std::string password;
    std::string token;
    std::string kerberosKeytab;
    ResourceV1Resource oem;
    std::string encryptionKey;
    bool encryptionKeySet;
};
struct AccountServiceV1LDAPSearchSettings
{
    std::string baseDistinguishedNames;
    std::string usernameAttribute;
    std::string groupNameAttribute;
    std::string groupsAttribute;
};
struct AccountServiceV1LDAPService
{
    AccountServiceV1LDAPSearchSettings searchSettings;
    ResourceV1Resource oem;
};
struct AccountServiceV1RoleMapping
{
    std::string remoteGroup;
    std::string remoteUser;
    std::string localRole;
    ResourceV1Resource oem;
};
struct AccountServiceV1TACACSplusService
{
    std::string privilegeLevelArgument;
    AccountServiceV1TACACSplusPasswordExchangeProtocol
        passwordExchangeProtocols;
};
struct AccountServiceV1ExternalAccountProvider
{
    AccountServiceV1AccountProviderTypes accountProviderType;
    bool serviceEnabled;
    std::string serviceAddresses;
    AccountServiceV1Authentication authentication;
    AccountServiceV1LDAPService lDAPService;
    AccountServiceV1RoleMapping remoteRoleMapping;
    CertificateCollectionV1CertificateCollection certificates;
    bool passwordSet;
    AccountServiceV1TACACSplusService tACACSplusService;
    int64_t priority;
};
struct AccountServiceV1AccountService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    bool serviceEnabled;
    int64_t authFailureLoggingThreshold;
    int64_t minPasswordLength;
    int64_t maxPasswordLength;
    int64_t accountLockoutThreshold;
    int64_t accountLockoutDuration;
    int64_t accountLockoutCounterResetAfter;
    ManagerAccountCollectionV1ManagerAccountCollection accounts;
    RoleCollectionV1RoleCollection roles;
    PrivilegeRegistryV1PrivilegeRegistry privilegeMap;
    AccountServiceV1Actions actions;
    AccountServiceV1LocalAccountAuth localAccountAuth;
    AccountServiceV1ExternalAccountProvider LDAP;
    AccountServiceV1ExternalAccountProvider activeDirectory;
    ExternalAccountProviderCollectionV1ExternalAccountProviderCollection
        additionalExternalAccountProviders;
    bool accountLockoutCounterResetEnabled;
    AccountServiceV1ExternalAccountProvider tACACSplus;
    ManagerAccountV1ManagerAccount supportedAccountTypes;
    std::string supportedOEMAccountTypes;
    PrivilegesV1Privileges restrictedPrivileges;
    std::string restrictedOemPrivileges;
    int64_t passwordExpirationDays;
};
#endif
