#ifndef MANAGERNETWORKPROTOCOL_V1
#define MANAGERNETWORKPROTOCOL_V1

#include "CertificateCollection_v1.h"
#include "ManagerNetworkProtocol_v1.h"
#include "Resource_v1.h"

enum class ManagerNetworkProtocolV1NotifyIPv6Scope
{
    Link,
    Site,
    Organization,
};
enum class ManagerNetworkProtocolV1SNMPAuthenticationProtocols
{
    Account,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};
enum class ManagerNetworkProtocolV1SNMPCommunityAccessMode
{
    Full,
    Limited,
};
enum class ManagerNetworkProtocolV1SNMPEncryptionProtocols
{
    None,
    Account,
    CBC_DES,
    CFB128_AES128,
};
struct ManagerNetworkProtocolV1OemActions
{};
struct ManagerNetworkProtocolV1Actions
{
    ManagerNetworkProtocolV1OemActions oem;
};
struct ManagerNetworkProtocolV1EngineId
{
    std::string privateEnterpriseId;
    std::string enterpriseSpecificMethod;
    std::string architectureId;
};
struct ManagerNetworkProtocolV1HTTPSProtocol
{
    bool protocolEnabled;
    int64_t port;
    CertificateCollectionV1CertificateCollection certificates;
};
struct ManagerNetworkProtocolV1Protocol
{
    bool protocolEnabled;
    int64_t port;
};
struct ManagerNetworkProtocolV1SNMPCommunity
{
    std::string name;
    std::string communityString;
    ManagerNetworkProtocolV1SNMPCommunityAccessMode accessMode;
};
struct ManagerNetworkProtocolV1SNMPProtocol
{
    bool protocolEnabled;
    int64_t port;
    bool enableSNMPv1;
    bool enableSNMPv2c;
    bool enableSNMPv3;
    ManagerNetworkProtocolV1SNMPCommunity communityStrings;
    ManagerNetworkProtocolV1SNMPCommunityAccessMode communityAccessMode;
    ManagerNetworkProtocolV1EngineId engineId;
    bool hideCommunityStrings;
    ManagerNetworkProtocolV1SNMPAuthenticationProtocols authenticationProtocol;
    ManagerNetworkProtocolV1SNMPEncryptionProtocols encryptionProtocol;
};
struct ManagerNetworkProtocolV1SSDProtocol
{
    bool protocolEnabled;
    int64_t port;
    int64_t notifyMulticastIntervalSeconds;
    int64_t notifyTTL;
    ManagerNetworkProtocolV1NotifyIPv6Scope notifyIPv6Scope;
};
struct ManagerNetworkProtocolV1NTPProtocol
{
    bool protocolEnabled;
    int64_t port;
    std::string nTPServers;
};
struct ManagerNetworkProtocolV1ManagerNetworkProtocol
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string hostName;
    std::string FQDN;
    ManagerNetworkProtocolV1Protocol HTTP;
    ManagerNetworkProtocolV1HTTPSProtocol HTTPS;
    ManagerNetworkProtocolV1SNMPProtocol SNMP;
    ManagerNetworkProtocolV1Protocol virtualMedia;
    ManagerNetworkProtocolV1Protocol telnet;
    ManagerNetworkProtocolV1SSDProtocol SSDP;
    ManagerNetworkProtocolV1Protocol IPMI;
    ManagerNetworkProtocolV1Protocol SSH;
    ManagerNetworkProtocolV1Protocol KVMIP;
    ResourceV1Resource status;
    ManagerNetworkProtocolV1Protocol DHCP;
    ManagerNetworkProtocolV1NTPProtocol NTP;
    ManagerNetworkProtocolV1Actions actions;
    ManagerNetworkProtocolV1Protocol dHCPv6;
    ManagerNetworkProtocolV1Protocol RDP;
    ManagerNetworkProtocolV1Protocol RFB;
};
#endif
