#pragma once
#include <nlohmann/json.hpp>

namespace manager_network_protocol
{
// clang-format off

enum class NotifyIPv6Scope{
    Invalid,
    Link,
    Site,
    Organization,
};

enum class SNMPCommunityAccessMode{
    Invalid,
    Full,
    Limited,
};

enum class SNMPAuthenticationProtocols{
    Invalid,
    Account,
    CommunityString,
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
    Account,
    CBC_DES,
    CFB128_AES128,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NotifyIPv6Scope, {
    {NotifyIPv6Scope::Invalid, "Invalid"},
    {NotifyIPv6Scope::Link, "Link"},
    {NotifyIPv6Scope::Site, "Site"},
    {NotifyIPv6Scope::Organization, "Organization"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPCommunityAccessMode, {
    {SNMPCommunityAccessMode::Invalid, "Invalid"},
    {SNMPCommunityAccessMode::Full, "Full"},
    {SNMPCommunityAccessMode::Limited, "Limited"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPAuthenticationProtocols, {
    {SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {SNMPAuthenticationProtocols::Account, "Account"},
    {SNMPAuthenticationProtocols::CommunityString, "CommunityString"},
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
    {SNMPEncryptionProtocols::Account, "Account"},
    {SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

}
// clang-format on
