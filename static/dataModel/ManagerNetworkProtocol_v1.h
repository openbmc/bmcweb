#ifndef MANAGERNETWORKPROTOCOL_V1
#define MANAGERNETWORKPROTOCOL_V1

#include "CertificateCollection_v1.h"
#include "ManagerNetworkProtocol_v1.h"
#include "Resource_v1.h"

enum class ManagerNetworkProtocol_v1_NotifyIPv6Scope {
    Link,
    Site,
    Organization,
};
enum class ManagerNetworkProtocol_v1_SNMPAuthenticationProtocols {
    Account,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
};
enum class ManagerNetworkProtocol_v1_SNMPCommunityAccessMode {
    Full,
    Limited,
};
enum class ManagerNetworkProtocol_v1_SNMPEncryptionProtocols {
    None,
    Account,
    CBC_DES,
    CFB128_AES128,
};
struct ManagerNetworkProtocol_v1_Actions
{
    ManagerNetworkProtocol_v1_OemActions oem;
};
struct ManagerNetworkProtocol_v1_EngineId
{
    std::string privateEnterpriseId;
    std::string enterpriseSpecificMethod;
    std::string architectureId;
};
struct ManagerNetworkProtocol_v1_HTTPSProtocol
{
    bool protocolEnabled;
    int64_t port;
    CertificateCollection_v1_CertificateCollection certificates;
};
struct ManagerNetworkProtocol_v1_ManagerNetworkProtocol
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string hostName;
    std::string FQDN;
    ManagerNetworkProtocol_v1_Protocol HTTP;
    ManagerNetworkProtocol_v1_HTTPSProtocol HTTPS;
    ManagerNetworkProtocol_v1_SNMPProtocol SNMP;
    ManagerNetworkProtocol_v1_Protocol virtualMedia;
    ManagerNetworkProtocol_v1_Protocol telnet;
    ManagerNetworkProtocol_v1_SSDProtocol SSDP;
    ManagerNetworkProtocol_v1_Protocol IPMI;
    ManagerNetworkProtocol_v1_Protocol SSH;
    ManagerNetworkProtocol_v1_Protocol KVMIP;
    Resource_v1_Resource status;
    ManagerNetworkProtocol_v1_Protocol DHCP;
    ManagerNetworkProtocol_v1_NTPProtocol NTP;
    ManagerNetworkProtocol_v1_Actions actions;
    ManagerNetworkProtocol_v1_Protocol dHCPv6;
    ManagerNetworkProtocol_v1_Protocol RDP;
    ManagerNetworkProtocol_v1_Protocol RFB;
};
struct ManagerNetworkProtocol_v1_NTPProtocol
{
    bool protocolEnabled;
    int64_t port;
    std::string nTPServers;
};
struct ManagerNetworkProtocol_v1_OemActions
{
};
struct ManagerNetworkProtocol_v1_Protocol
{
    bool protocolEnabled;
    int64_t port;
};
struct ManagerNetworkProtocol_v1_SNMPCommunity
{
    std::string name;
    std::string communityString;
    ManagerNetworkProtocol_v1_SNMPCommunityAccessMode accessMode;
};
struct ManagerNetworkProtocol_v1_SNMPProtocol
{
    bool protocolEnabled;
    int64_t port;
    bool enableSNMPv1;
    bool enableSNMPv2c;
    bool enableSNMPv3;
    ManagerNetworkProtocol_v1_SNMPCommunity communityStrings;
    ManagerNetworkProtocol_v1_SNMPCommunityAccessMode communityAccessMode;
    ManagerNetworkProtocol_v1_EngineId engineId;
    bool hideCommunityStrings;
    ManagerNetworkProtocol_v1_SNMPAuthenticationProtocols authenticationProtocol;
    ManagerNetworkProtocol_v1_SNMPEncryptionProtocols encryptionProtocol;
};
struct ManagerNetworkProtocol_v1_SSDProtocol
{
    bool protocolEnabled;
    int64_t port;
    int64_t notifyMulticastIntervalSeconds;
    int64_t notifyTTL;
    ManagerNetworkProtocol_v1_NotifyIPv6Scope notifyIPv6Scope;
};
#endif
