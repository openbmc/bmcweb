#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace allow_deny
{
// clang-format off

enum class IPAddressType{
    Invalid,
    IPv4,
    IPv6,
};

enum class AllowType{
    Invalid,
    Allow,
    Deny,
};

enum class DataDirection{
    Invalid,
    Ingress,
    Egress,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IPAddressType, {
    {IPAddressType::Invalid, "Invalid"},
    {IPAddressType::IPv4, "IPv4"},
    {IPAddressType::IPv6, "IPv6"},
});

BOOST_DESCRIBE_ENUM(IPAddressType,

    Invalid,
    IPv4,
    IPv6,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AllowType, {
    {AllowType::Invalid, "Invalid"},
    {AllowType::Allow, "Allow"},
    {AllowType::Deny, "Deny"},
});

BOOST_DESCRIBE_ENUM(AllowType,

    Invalid,
    Allow,
    Deny,
);

NLOHMANN_JSON_SERIALIZE_ENUM(DataDirection, {
    {DataDirection::Invalid, "Invalid"},
    {DataDirection::Ingress, "Ingress"},
    {DataDirection::Egress, "Egress"},
});

BOOST_DESCRIBE_ENUM(DataDirection,

    Invalid,
    Ingress,
    Egress,
);

}
// clang-format on
