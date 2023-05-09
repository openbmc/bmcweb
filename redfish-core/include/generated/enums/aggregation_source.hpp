#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace aggregation_source
{
// clang-format off

enum class SNMPAuthenticationProtocols{
    Invalid,
    None,
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
    CBC_DES,
    CFB128_AES128,
    CFB128_AES192,
    CFB128_AES256,
};

enum class AggregationType{
    Invalid,
    NotificationsOnly,
    Full,
};

enum class UserAuthenticationMethod{
    Invalid,
    PublicKey,
    Password,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPAuthenticationProtocols, {
    {SNMPAuthenticationProtocols::Invalid, "Invalid"},
    {SNMPAuthenticationProtocols::None, "None"},
    {SNMPAuthenticationProtocols::CommunityString, "CommunityString"},
    {SNMPAuthenticationProtocols::HMAC_MD5, "HMAC_MD5"},
    {SNMPAuthenticationProtocols::HMAC_SHA96, "HMAC_SHA96"},
    {SNMPAuthenticationProtocols::HMAC128_SHA224, "HMAC128_SHA224"},
    {SNMPAuthenticationProtocols::HMAC192_SHA256, "HMAC192_SHA256"},
    {SNMPAuthenticationProtocols::HMAC256_SHA384, "HMAC256_SHA384"},
    {SNMPAuthenticationProtocols::HMAC384_SHA512, "HMAC384_SHA512"},
});

BOOST_DESCRIBE_ENUM(SNMPAuthenticationProtocols,

    Invalid,
    None,
    CommunityString,
    HMAC_MD5,
    HMAC_SHA96,
    HMAC128_SHA224,
    HMAC192_SHA256,
    HMAC256_SHA384,
    HMAC384_SHA512,
);

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPEncryptionProtocols, {
    {SNMPEncryptionProtocols::Invalid, "Invalid"},
    {SNMPEncryptionProtocols::None, "None"},
    {SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
    {SNMPEncryptionProtocols::CFB128_AES192, "CFB128_AES192"},
    {SNMPEncryptionProtocols::CFB128_AES256, "CFB128_AES256"},
});

BOOST_DESCRIBE_ENUM(SNMPEncryptionProtocols,

    Invalid,
    None,
    CBC_DES,
    CFB128_AES128,
    CFB128_AES192,
    CFB128_AES256,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AggregationType, {
    {AggregationType::Invalid, "Invalid"},
    {AggregationType::NotificationsOnly, "NotificationsOnly"},
    {AggregationType::Full, "Full"},
});

BOOST_DESCRIBE_ENUM(AggregationType,

    Invalid,
    NotificationsOnly,
    Full,
);

NLOHMANN_JSON_SERIALIZE_ENUM(UserAuthenticationMethod, {
    {UserAuthenticationMethod::Invalid, "Invalid"},
    {UserAuthenticationMethod::PublicKey, "PublicKey"},
    {UserAuthenticationMethod::Password, "Password"},
});

BOOST_DESCRIBE_ENUM(UserAuthenticationMethod,

    Invalid,
    PublicKey,
    Password,
);

}
// clang-format on
