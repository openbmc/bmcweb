#pragma once
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

enum class SSHKeyType{
    Invalid,
    RSA,
    DSA,
    ECDSA,
    Ed25519,
};

enum class ECDSACurveType{
    Invalid,
    NISTP256,
    NISTP384,
    NISTP521,
    NISTK163,
    NISTP192,
    NISTP224,
    NISTK233,
    NISTB233,
    NISTK283,
    NISTK409,
    NISTB409,
    NISTT571,
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

NLOHMANN_JSON_SERIALIZE_ENUM(SNMPEncryptionProtocols, {
    {SNMPEncryptionProtocols::Invalid, "Invalid"},
    {SNMPEncryptionProtocols::None, "None"},
    {SNMPEncryptionProtocols::CBC_DES, "CBC_DES"},
    {SNMPEncryptionProtocols::CFB128_AES128, "CFB128_AES128"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(AggregationType, {
    {AggregationType::Invalid, "Invalid"},
    {AggregationType::NotificationsOnly, "NotificationsOnly"},
    {AggregationType::Full, "Full"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UserAuthenticationMethod, {
    {UserAuthenticationMethod::Invalid, "Invalid"},
    {UserAuthenticationMethod::PublicKey, "PublicKey"},
    {UserAuthenticationMethod::Password, "Password"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(SSHKeyType, {
    {SSHKeyType::Invalid, "Invalid"},
    {SSHKeyType::RSA, "RSA"},
    {SSHKeyType::DSA, "DSA"},
    {SSHKeyType::ECDSA, "ECDSA"},
    {SSHKeyType::Ed25519, "Ed25519"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ECDSACurveType, {
    {ECDSACurveType::Invalid, "Invalid"},
    {ECDSACurveType::NISTP256, "NISTP256"},
    {ECDSACurveType::NISTP384, "NISTP384"},
    {ECDSACurveType::NISTP521, "NISTP521"},
    {ECDSACurveType::NISTK163, "NISTK163"},
    {ECDSACurveType::NISTP192, "NISTP192"},
    {ECDSACurveType::NISTP224, "NISTP224"},
    {ECDSACurveType::NISTK233, "NISTK233"},
    {ECDSACurveType::NISTB233, "NISTB233"},
    {ECDSACurveType::NISTK283, "NISTK283"},
    {ECDSACurveType::NISTK409, "NISTK409"},
    {ECDSACurveType::NISTB409, "NISTB409"},
    {ECDSACurveType::NISTT571, "NISTT571"},
});

}
// clang-format on
