#ifndef EXTERNALACCOUNTPROVIDER_V1
#define EXTERNALACCOUNTPROVIDER_V1

#include "CertificateCollection_v1.h"
#include "ExternalAccountProvider_v1.h"
#include "Resource_v1.h"

enum class ExternalAccountProviderV1AccountProviderTypes
{
    RedfishService,
    ActiveDirectoryService,
    LDAPService,
    OEM,
};
enum class ExternalAccountProviderV1AuthenticationTypes
{
    Token,
    KerberosKeytab,
    UsernameAndPassword,
    OEM,
};
enum class ExternalAccountProviderV1TACACSplusPasswordExchangeProtocol
{
    ASCII,
    PAP,
    CHAP,
    MSCHAPv1,
    MSCHAPv2,
};
struct ExternalAccountProviderV1OemActions
{};
struct ExternalAccountProviderV1Actions
{
    ExternalAccountProviderV1OemActions oem;
};
struct ExternalAccountProviderV1Authentication
{
    ExternalAccountProviderV1AuthenticationTypes authenticationType;
    std::string username;
    std::string password;
    std::string token;
    std::string kerberosKeytab;
    ResourceV1Resource oem;
    std::string encryptionKey;
    bool encryptionKeySet;
};
struct ExternalAccountProviderV1LDAPSearchSettings
{
    std::string baseDistinguishedNames;
    std::string usernameAttribute;
    std::string groupNameAttribute;
    std::string groupsAttribute;
};
struct ExternalAccountProviderV1LDAPService
{
    ExternalAccountProviderV1LDAPSearchSettings searchSettings;
    ResourceV1Resource oem;
};
struct ExternalAccountProviderV1RoleMapping
{
    std::string remoteGroup;
    std::string remoteUser;
    std::string localRole;
    ResourceV1Resource oem;
};
struct ExternalAccountProviderV1Links
{
    ResourceV1Resource oem;
};
struct ExternalAccountProviderV1TACACSplusService
{
    std::string privilegeLevelArgument;
    ExternalAccountProviderV1TACACSplusPasswordExchangeProtocol
        passwordExchangeProtocols;
};
struct ExternalAccountProviderV1ExternalAccountProvider
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ExternalAccountProviderV1AccountProviderTypes accountProviderType;
    bool serviceEnabled;
    std::string serviceAddresses;
    ExternalAccountProviderV1Authentication authentication;
    ExternalAccountProviderV1LDAPService lDAPService;
    ExternalAccountProviderV1RoleMapping remoteRoleMapping;
    ExternalAccountProviderV1Links links;
    ExternalAccountProviderV1Actions actions;
    CertificateCollectionV1CertificateCollection certificates;
    ExternalAccountProviderV1TACACSplusService tACACSplusService;
    int64_t priority;
};
#endif
