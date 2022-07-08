#pragma once
#include <nlohmann/json.hpp>

namespace connection_method
{
// clang-format off

enum class ConnectionMethodType{
    Invalid,
    Redfish,
    SNMP,
    IPMI15,
    IPMI20,
    NETCONF,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectionMethodType, {
    {ConnectionMethodType::Invalid, "Invalid"},
    {ConnectionMethodType::Redfish, "Redfish"},
    {ConnectionMethodType::SNMP, "SNMP"},
    {ConnectionMethodType::IPMI15, "IPMI15"},
    {ConnectionMethodType::IPMI20, "IPMI20"},
    {ConnectionMethodType::NETCONF, "NETCONF"},
    {ConnectionMethodType::OEM, "OEM"},
});

}
// clang-format on
