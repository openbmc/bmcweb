#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace ip_addresses
{
// clang-format off

enum class IPv4AddressOrigin{
    Invalid,
    Static,
    DHCP,
    BOOTP,
    IPv4LinkLocal,
};

enum class IPv6AddressOrigin{
    Invalid,
    Static,
    DHCPv6,
    LinkLocal,
    SLAAC,
};

enum class AddressState{
    Invalid,
    Preferred,
    Deprecated,
    Tentative,
    Failed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IPv4AddressOrigin, {
    {IPv4AddressOrigin::Invalid, "Invalid"},
    {IPv4AddressOrigin::Static, "Static"},
    {IPv4AddressOrigin::DHCP, "DHCP"},
    {IPv4AddressOrigin::BOOTP, "BOOTP"},
    {IPv4AddressOrigin::IPv4LinkLocal, "IPv4LinkLocal"},
});

BOOST_DESCRIBE_ENUM(IPv4AddressOrigin,

    Invalid,
    Static,
    DHCP,
    BOOTP,
    IPv4LinkLocal,
);

NLOHMANN_JSON_SERIALIZE_ENUM(IPv6AddressOrigin, {
    {IPv6AddressOrigin::Invalid, "Invalid"},
    {IPv6AddressOrigin::Static, "Static"},
    {IPv6AddressOrigin::DHCPv6, "DHCPv6"},
    {IPv6AddressOrigin::LinkLocal, "LinkLocal"},
    {IPv6AddressOrigin::SLAAC, "SLAAC"},
});

BOOST_DESCRIBE_ENUM(IPv6AddressOrigin,

    Invalid,
    Static,
    DHCPv6,
    LinkLocal,
    SLAAC,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AddressState, {
    {AddressState::Invalid, "Invalid"},
    {AddressState::Preferred, "Preferred"},
    {AddressState::Deprecated, "Deprecated"},
    {AddressState::Tentative, "Tentative"},
    {AddressState::Failed, "Failed"},
});

BOOST_DESCRIBE_ENUM(AddressState,

    Invalid,
    Preferred,
    Deprecated,
    Tentative,
    Failed,
);

}
// clang-format on
