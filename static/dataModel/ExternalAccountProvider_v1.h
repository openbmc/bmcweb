#ifndef EXTERNALACCOUNTPROVIDER_V1
#define EXTERNALACCOUNTPROVIDER_V1

#include "CertificateCollection_v1.h"
#include "ExternalAccountProvider_v1.h"
#include "Resource_v1.h"

enum class ExternalAccountProvider_v1_AccountProviderTypes {
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
};
enum class ExternalAccountProvider_v1_AuthenticationTypes {
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
};
struct ExternalAccountProvider_v1_Actions
{
    ExternalAccountProvider_v1_OemActions oem;
};
struct ExternalAccountProvider_v1_Authentication
{
    ExternalAccountProvider_v1_AuthenticationTypes authenticationType;
    std::string username;
    std::string password;
    std::string token;
    std::string kerberosKeytab;
    Resource_v1_Resource oem;
};
struct ExternalAccountProvider_v1_ExternalAccountProvider
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ExternalAccountProvider_v1_AccountProviderTypes accountProviderType;
    bool serviceEnabled;
    std::string serviceAddresses;
    ExternalAccountProvider_v1_Authentication authentication;
    ExternalAccountProvider_v1_LDAPService lDAPService;
    ExternalAccountProvider_v1_RoleMapping remoteRoleMapping;
    ExternalAccountProvider_v1_Links links;
    ExternalAccountProvider_v1_Actions actions;
    CertificateCollection_v1_CertificateCollection certificates;
};
struct ExternalAccountProvider_v1_LDAPSearchSettings
{
    std::string baseDistinguishedNames;
    std::string usernameAttribute;
    std::string groupNameAttribute;
    std::string groupsAttribute;
};
struct ExternalAccountProvider_v1_LDAPService
{
    ExternalAccountProvider_v1_LDAPSearchSettings searchSettings;
    Resource_v1_Resource oem;
};
struct ExternalAccountProvider_v1_Links
{
    Resource_v1_Resource oem;
};
struct ExternalAccountProvider_v1_OemActions
{
};
struct ExternalAccountProvider_v1_RoleMapping
{
    std::string remoteGroup;
    std::string remoteUser;
    std::string localRole;
    Resource_v1_Resource oem;
};
#endif
