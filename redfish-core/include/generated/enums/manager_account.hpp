#pragma once
#include <nlohmann/json.hpp>

namespace manager_account
{
// clang-format off

enum class AccountTypes{
    Invalid,
    Redfish,
    SNMP,
    OEM,
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    VirtualMedia,
    WebUI,
};

enum class SNMPAuthenticationProtocols{
    Invalid,
    None,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
};

enum class SNMPEncryptionProtocols{
    Invalid,
    None,
    CBC_DES,
    CFB128_AES128,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AccountTypes, {
    {AccountTypes::Invalid, "Invalid"},
    {AccountTypes::Redfish, "Redfish"},
    {AccountTypes::SNMP, "SNMP"},
    {AccountTypes::OEM, "OEM"},
    {AccountTypes::HostConsole, "HostConsole"},
    {AccountTypes::ManagerConsole, "ManagerConsole"},
    {AccountTypes::IPMI, "IPMI"},
    {AccountTypes::KVMIP, "KVMIP"},
    {AccountTypes::VirtualMedia, "VirtualMedia"},
    {AccountTypes::WebUI, "WebUI"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPAuthenticationProtocols, {
    {SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {SNMPAuthenticationProtocols::None, "None"},
    {SNMPAuthenticationProtocols::HMAC_MD5, "HMAC_MD5"},
    {SNMPAuthenticationProtocols::HMAC_SHA96, "HMAC_SHA96"},
    {SNMPAuthenticationProtocols::HMAC128_SHA224, "HMAC128_SHA224"},
    {SNMPAuthenticationProtocols::HMAC192_SHA256, "HMAC192_SHA256"},
    {SNMPAuthenticationProtocols::HMAC256_SHA384, "HMAC256_SHA384"},
    {SNMPAuthenticationProtocols::HMAC384_SHA512, "HMAC384_SHA512"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPEncryptionProtocols, {
    {SNMPEncryptionProtocols::Invalid, "Invalid"},
    {SNMPEncryptionProtocols::None, "None"},
    {SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

}
// clang-format on
