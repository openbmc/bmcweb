#ifndef ACCOUNTSERVICE_V1
#define ACCOUNTSERVICE_V1

#include "AccountService_v1.h"
#include "CertificateCollection_v1.h"
#include "ExternalAccountProviderCollection_v1.h"
#include "ManagerAccountCollection_v1.h"
#include "PrivilegeRegistry_v1.h"
#include "Resource_v1.h"
#include "RoleCollection_v1.h"

enum class AccountService_v1_AccountProviderTypes {
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
};
enum class AccountService_v1_AuthenticationTypes {
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
};
enum class AccountService_v1_LocalAccountAuth {
    Enabled,
    Disabled,
    Fallback,
    LocalFirst,
};
struct AccountService_v1_AccountService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    bool serviceEnabled;
    int64_t authFailureLoggingThreshold;
    int64_t minPasswordLength;
    int64_t maxPasswordLength;
    int64_t accountLockoutThreshold;
    int64_t accountLockoutDuration;
    int64_t accountLockoutCounterResetAfter;
    ManagerAccountCollection_v1_ManagerAccountCollection accounts;
    RoleCollection_v1_RoleCollection roles;
    PrivilegeRegistry_v1_PrivilegeRegistry privilegeMap;
    AccountService_v1_Actions actions;
    AccountService_v1_LocalAccountAuth localAccountAuth;
    AccountService_v1_ExternalAccountProvider LDAP;
    AccountService_v1_ExternalAccountProvider activeDirectory;
    ExternalAccountProviderCollection_v1_ExternalAccountProviderCollection additionalExternalAccountProviders;
    bool accountLockoutCounterResetEnabled;
};
struct AccountService_v1_Actions
{
    AccountService_v1_OemActions oem;
};
struct AccountService_v1_Authentication
{
    AccountService_v1_AuthenticationTypes authenticationType;
    std::string username;
    std::string password;
    std::string token;
    std::string kerberosKeytab;
    Resource_v1_Resource oem;
};
struct AccountService_v1_ExternalAccountProvider
{
    AccountService_v1_AccountProviderTypes accountProviderType;
    bool serviceEnabled;
    std::string serviceAddresses;
    AccountService_v1_Authentication authentication;
    AccountService_v1_LDAPService lDAPService;
    AccountService_v1_RoleMapping remoteRoleMapping;
    CertificateCollection_v1_CertificateCollection certificates;
    bool passwordSet;
};
struct AccountService_v1_LDAPSearchSettings
{
    std::string baseDistinguishedNames;
    std::string usernameAttribute;
    std::string groupNameAttribute;
    std::string groupsAttribute;
};
struct AccountService_v1_LDAPService
{
    AccountService_v1_LDAPSearchSettings searchSettings;
    Resource_v1_Resource oem;
};
struct AccountService_v1_OemActions
{
};
struct AccountService_v1_RoleMapping
{
    std::string remoteGroup;
    std::string remoteUser;
    std::string localRole;
    Resource_v1_Resource oem;
};
#endif
