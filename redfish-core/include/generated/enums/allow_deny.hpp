#pragma once
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

NLOHMANN_JSON_SERIALIZE_ENUM(AllowType, {
    {AllowType::Invalid, "Invalid"},
    {AllowType::Allow, "Allow"},
    {AllowType::Deny, "Deny"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DataDirection, {
    {DataDirection::Invalid, "Invalid"},
    {DataDirection::Ingress, "Ingress"},
    {DataDirection::Egress, "Egress"},
});

}
// clang-format on
