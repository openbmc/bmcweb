// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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
    None,
};

enum class SNMPEncryptionProtocols{
    Invalid,
    None,
    Account,
    CBC_DES,
    CFB128_AES128,
    CFB128_AES192,
    CFB128_AES256,
};

enum class IPv6AddressGenerationMode{
    Invalid,
    StablePrivacy,
    EUI64,
};

enum class SSHPreferredAuthentication{
    Invalid,
    Password,
    PublicKey,
    KeyboardInteractive,
    GSSAPIWithMIC,
    HostBased,
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
    {SNMPAuthenticationProtocols::None, "None"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPEncryptionProtocols, {
    {SNMPEncryptionProtocols::Invalid, "Invalid"},
    {SNMPEncryptionProtocols::None, "None"},
    {SNMPEncryptionProtocols::Account, "Account"},
    {SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
    {SNMPEncryptionProtocols::CFB128_AES192, "CFB128_AES192"},
    {SNMPEncryptionProtocols::CFB128_AES256, "CFB128_AES256"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(IPv6AddressGenerationMode, {
    {IPv6AddressGenerationMode::Invalid, "Invalid"},
    {IPv6AddressGenerationMode::StablePrivacy, "StablePrivacy"},
    {IPv6AddressGenerationMode::EUI64, "EUI64"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SSHPreferredAuthentication, {
    {SSHPreferredAuthentication::Invalid, "Invalid"},
    {SSHPreferredAuthentication::Password, "Password"},
    {SSHPreferredAuthentication::PublicKey, "PublicKey"},
    {SSHPreferredAuthentication::KeyboardInteractive, "KeyboardInteractive"},
    {SSHPreferredAuthentication::GSSAPIWithMIC, "GSSAPIWithMIC"},
    {SSHPreferredAuthentication::HostBased, "HostBased"},
});

// clang-format on
} // namespace manager_network_protocol
