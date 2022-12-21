#pragma once
#include <nlohmann/json.hpp>

namespace network_device_function
{
// clang-format off

enum class NetworkDeviceTechnology{
    Invalid,
    Disabled,
    Ethernet,
    FibreChannel,
    iSCSI,
    FibreChannelOverEthernet,
    InfiniBand,
};

enum class IPAddressType{
    Invalid,
    IPv4,
    IPv6,
};

enum class AuthenticationMethod{
    Invalid,
    None,
    CHAP,
    MutualCHAP,
};

enum class WWNSource{
    Invalid,
    ConfiguredLocally,
    ProvidedByFabric,
};

enum class BootMode{
    Invalid,
    Disabled,
    PXE,
    iSCSI,
    FibreChannel,
    FibreChannelOverEthernet,
    HTTP,
};

enum class DataDirection{
    Invalid,
    None,
    Ingress,
    Egress,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NetworkDeviceTechnology, {
    {NetworkDeviceTechnology::Invalid, "Invalid"},
    {NetworkDeviceTechnology::Disabled, "Disabled"},
    {NetworkDeviceTechnology::Ethernet, "Ethernet"},
    {NetworkDeviceTechnology::FibreChannel, "FibreChannel"},
    {NetworkDeviceTechnology::iSCSI, "iSCSI"},
    {NetworkDeviceTechnology::FibreChannelOverEthernet, "FibreChannelOverEthernet"},
    {NetworkDeviceTechnology::InfiniBand, "InfiniBand"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(IPAddressType, {
    {IPAddressType::Invalid, "Invalid"},
    {IPAddressType::IPv4, "IPv4"},
    {IPAddressType::IPv6, "IPv6"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AuthenticationMethod, {
    {AuthenticationMethod::Invalid, "Invalid"},
    {AuthenticationMethod::None, "None"},
    {AuthenticationMethod::CHAP, "CHAP"},
    {AuthenticationMethod::MutualCHAP, "MutualCHAP"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(WWNSource, {
    {WWNSource::Invalid, "Invalid"},
    {WWNSource::ConfiguredLocally, "ConfiguredLocally"},
    {WWNSource::ProvidedByFabric, "ProvidedByFabric"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BootMode, {
    {BootMode::Invalid, "Invalid"},
    {BootMode::Disabled, "Disabled"},
    {BootMode::PXE, "PXE"},
    {BootMode::iSCSI, "iSCSI"},
    {BootMode::FibreChannel, "FibreChannel"},
    {BootMode::FibreChannelOverEthernet, "FibreChannelOverEthernet"},
    {BootMode::HTTP, "HTTP"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DataDirection, {
    {DataDirection::Invalid, "Invalid"},
    {DataDirection::None, "None"},
    {DataDirection::Ingress, "Ingress"},
    {DataDirection::Egress, "Egress"},
});

}
// clang-format on
