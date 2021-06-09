#ifndef IPADDRESSES_V1
#define IPADDRESSES_V1

#include "IPAddresses_v1.h"
#include "Resource_v1.h"

enum class IPAddressesV1AddressState
{
    Preferred,
    Deprecated,
    Tentative,
    Failed,
};
enum class IPAddressesV1IPv4AddressOrigin
{
    Static,
    DHCP,
    BOOTP,
    IPv4LinkLocal,
};
enum class IPAddressesV1IPv6AddressOrigin
{
    Static,
    DHCPv6,
    LinkLocal,
    SLAAC,
};
struct IPAddressesV1IPv4Address
{
    ResourceV1Resource oem;
    std::string address;
    std::string subnetMask;
    IPAddressesV1IPv4AddressOrigin addressOrigin;
    std::string gateway;
};
struct IPAddressesV1IPv6Address
{
    ResourceV1Resource oem;
    std::string address;
    int64_t prefixLength;
    IPAddressesV1IPv6AddressOrigin addressOrigin;
    IPAddressesV1AddressState addressState;
};
struct IPAddressesV1IPv6GatewayStaticAddress
{
    ResourceV1Resource oem;
    std::string address;
    int64_t prefixLength;
};
struct IPAddressesV1IPv6StaticAddress
{
    ResourceV1Resource oem;
    std::string address;
    int64_t prefixLength;
};
#endif
