#ifndef ETHERNETINTERFACE_V1
#define ETHERNETINTERFACE_V1

#include "EthernetInterface_v1.h"
#include "IPAddresses_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "VLanNetworkInterfaceCollection_v1.h"
#include "VLanNetworkInterface_v1.h"

enum class EthernetInterfaceV1DHCPFallback
{
    Static,
    AutoConfig,
    None,
};
enum class EthernetInterfaceV1DHCPv6OperatingMode
{
    Stateful,
    Stateless,
    Disabled,
};
enum class EthernetInterfaceV1EthernetDeviceType
{
    Physical,
    Virtual,
};
enum class EthernetInterfaceV1LinkStatus
{
    LinkUp,
    NoLink,
    LinkDown,
};
struct EthernetInterfaceV1OemActions
{};
struct EthernetInterfaceV1Actions
{
    EthernetInterfaceV1OemActions oem;
};
struct EthernetInterfaceV1DHCPv4Configuration
{
    bool dHCPEnabled;
    bool useDNSServers;
    bool useDomainName;
    bool useGateway;
    bool useNTPServers;
    bool useStaticRoutes;
    EthernetInterfaceV1DHCPFallback fallbackAddress;
};
struct EthernetInterfaceV1DHCPv6Configuration
{
    EthernetInterfaceV1DHCPv6OperatingMode operatingMode;
    bool useDNSServers;
    bool useDomainName;
    bool useNTPServers;
    bool useRapidCommit;
};
struct EthernetInterfaceV1IPv6AddressPolicyEntry
{
    std::string prefix;
    int64_t precedence;
    int64_t label;
};
struct EthernetInterfaceV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ endpoints;
    NavigationReference_ hostInterface;
    NavigationReference_ chassis;
    NavigationReference_ networkDeviceFunction;
};
struct EthernetInterfaceV1StatelessAddressAutoConfiguration
{
    bool iPv4AutoConfigEnabled;
    bool iPv6AutoConfigEnabled;
};
struct EthernetInterfaceV1EthernetInterface
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string uefiDevicePath;
    ResourceV1Resource status;
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
    VLanNetworkInterfaceV1VLanNetworkInterface VLAN;
    IPAddressesV1IPAddresses iPv4Addresses;
    EthernetInterfaceV1IPv6AddressPolicyEntry iPv6AddressPolicyTable;
    IPAddressesV1IPAddresses iPv6Addresses;
    IPAddressesV1IPAddresses iPv6StaticAddresses;
    std::string iPv6DefaultGateway;
    std::string nameServers;
    VLanNetworkInterfaceCollectionV1VLanNetworkInterfaceCollection vLANs;
    EthernetInterfaceV1LinkStatus linkStatus;
    EthernetInterfaceV1Links links;
    EthernetInterfaceV1Actions actions;
    EthernetInterfaceV1DHCPv4Configuration dHCPv4;
    EthernetInterfaceV1DHCPv6Configuration dHCPv6;
    EthernetInterfaceV1StatelessAddressAutoConfiguration
        statelessAddressAutoConfig;
    IPAddressesV1IPAddresses iPv6StaticDefaultGateways;
    std::string staticNameServers;
    IPAddressesV1IPAddresses iPv4StaticAddresses;
    EthernetInterfaceV1EthernetDeviceType ethernetInterfaceType;
};
#endif
