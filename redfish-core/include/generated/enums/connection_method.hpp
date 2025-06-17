// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
    ModbusSerial,
    ModbusTCP,
};

enum class TunnelingProtocolType{
    Invalid,
    SSH,
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
    {ConnectionMethodType::ModbusSerial, "ModbusSerial"},
    {ConnectionMethodType::ModbusTCP, "ModbusTCP"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TunnelingProtocolType, {
    {TunnelingProtocolType::Invalid, "Invalid"},
    {TunnelingProtocolType::SSH, "SSH"},
    {TunnelingProtocolType::OEM, "OEM"},
});

}
// clang-format on
