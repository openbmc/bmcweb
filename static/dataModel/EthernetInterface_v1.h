#ifndef ETHERNETINTERFACE_V1
#define ETHERNETINTERFACE_V1

#include "EthernetInterface_v1.h"
#include "IPAddresses_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "VLanNetworkInterfaceCollection_v1.h"
#include "VLanNetworkInterface_v1.h"

enum class EthernetInterface_v1_DHCPFallback
{
    Static,
    AutoConfig,
    None,
};
enum class EthernetInterface_v1_DHCPv6OperatingMode
{
    Stateful,
    Stateless,
    Disabled,
};
enum class EthernetInterface_v1_EthernetDeviceType
{
    Physical,
    Virtual,
};
enum class EthernetInterface_v1_LinkStatus
{
    LinkUp,
    NoLink,
    LinkDown,
};
struct EthernetInterface_v1_Actions
{
    EthernetInterface_v1_OemActions oem;
};
struct EthernetInterface_v1_DHCPv4Configuration
{
    bool dHCPEnabled;
    bool useDNSServers;
    bool useDomainName;
    bool useGateway;
    bool useNTPServers;
    bool useStaticRoutes;
    EthernetInterface_v1_DHCPFallback fallbackAddress;
};
struct EthernetInterface_v1_DHCPv6Configuration
{
    EthernetInterface_v1_DHCPv6OperatingMode operatingMode;
    bool useDNSServers;
    bool useDomainName;
    bool useNTPServers;
    bool useRapidCommit;
};
struct EthernetInterface_v1_EthernetInterface
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string uefiDevicePath;
    Resource_v1_Resource status;
    bool interfaceEnabled;
    std::string permanentMACAddress;
    std::string mACAddress;
    int64_t speedMbps;
    bool autoNeg;
    bool fullDuplex;
    int64_t mTUSize;
    std::string hostName;
    std::string FQDN;
    int64_t maxIPv6StaticAddresses;
    VLanNetworkInterface_v1_VLanNetworkInterface VLAN;
    IPAddresses_v1_IPAddresses iPv4Addresses;
    EthernetInterface_v1_IPv6AddressPolicyEntry iPv6AddressPolicyTable;
    IPAddresses_v1_IPAddresses iPv6Addresses;
    IPAddresses_v1_IPAddresses iPv6StaticAddresses;
    std::string iPv6DefaultGateway;
    std::string nameServers;
    VLanNetworkInterfaceCollection_v1_VLanNetworkInterfaceCollection vLANs;
    EthernetInterface_v1_LinkStatus linkStatus;
    EthernetInterface_v1_Links links;
    EthernetInterface_v1_Actions actions;
    EthernetInterface_v1_DHCPv4Configuration dHCPv4;
    EthernetInterface_v1_DHCPv6Configuration dHCPv6;
    EthernetInterface_v1_StatelessAddressAutoConfiguration
        statelessAddressAutoConfig;
    IPAddresses_v1_IPAddresses iPv6StaticDefaultGateways;
    std::string staticNameServers;
    IPAddresses_v1_IPAddresses iPv4StaticAddresses;
    EthernetInterface_v1_EthernetDeviceType ethernetInterfaceType;
};
struct EthernetInterface_v1_IPv6AddressPolicyEntry
{
    std::string prefix;
    int64_t precedence;
    int64_t label;
};
struct EthernetInterface_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ hostInterface;
    NavigationReference__ chassis;
    NavigationReference__ networkDeviceFunction;
};
struct EthernetInterface_v1_OemActions
{};
struct EthernetInterface_v1_StatelessAddressAutoConfiguration
{
    bool iPv4AutoConfigEnabled;
    bool iPv6AutoConfigEnabled;
};
#endif
