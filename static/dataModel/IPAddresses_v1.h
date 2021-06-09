#ifndef IPADDRESSES_V1
#define IPADDRESSES_V1

#include "IPAddresses_v1.h"
#include "Resource_v1.h"

enum class IPAddresses_v1_AddressState
{
    Preferred,
    Deprecated,
    Tentative,
    Failed,
};
enum class IPAddresses_v1_IPv4AddressOrigin
{
    Static,
    DHCP,
    BOOTP,
    IPv4LinkLocal,
};
enum class IPAddresses_v1_IPv6AddressOrigin
{
    Static,
    DHCPv6,
    LinkLocal,
    SLAAC,
};
struct IPAddresses_v1_IPv4Address
{
    Resource_v1_Resource oem;
    std::string address;
    std::string subnetMask;
    IPAddresses_v1_IPv4AddressOrigin addressOrigin;
    std::string gateway;
};
struct IPAddresses_v1_IPv6Address
{
    Resource_v1_Resource oem;
    std::string address;
    int64_t prefixLength;
    IPAddresses_v1_IPv6AddressOrigin addressOrigin;
    IPAddresses_v1_AddressState addressState;
};
struct IPAddresses_v1_IPv6GatewayStaticAddress
{
    Resource_v1_Resource oem;
    std::string address;
    int64_t prefixLength;
};
struct IPAddresses_v1_IPv6StaticAddress
{
    Resource_v1_Resource oem;
    std::string address;
    int64_t prefixLength;
};
#endif
