#pragma once
#include <nlohmann/json.hpp>

namespace key_policy
{
// clang-format off

enum class KeyPolicyType{
    Invalid,
    NVMeoF,
};

enum class NVMeoFSecurityProtocolType{
    Invalid,
    DHHC,
    TLS_PSK,
    OEM,
};

enum class NVMeoFSecureHashType{
    Invalid,
    SHA256,
    SHA384,
    SHA512,
};

enum class NVMeoFSecurityTransportType{
    Invalid,
    TLSv2,
    TLSv3,
};

enum class NVMeoFCipherSuiteType{
    Invalid,
    TLS_AES_128_GCM_SHA256,
    TLS_AES_256_GCM_SHA384,
};

enum class NVMeoFDHGroupType{
    Invalid,
    FFDHE2048,
    FFDHE3072,
    FFDHE4096,
    FFDHE6144,
    FFDHE8192,
};

NLOHMANN_JSON_SERIALIZE_ENUM(KeyPolicyType, {
    {KeyPolicyType::Invalid, "Invalid"},
    {KeyPolicyType::NVMeoF, "NVMeoF"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NVMeoFSecurityProtocolType, {
    {NVMeoFSecurityProtocolType::Invalid, "Invalid"},
    {NVMeoFSecurityProtocolType::DHHC, "DHHC"},
    {NVMeoFSecurityProtocolType::TLS_PSK, "TLS_PSK"},
    {NVMeoFSecurityProtocolType::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NVMeoFSecureHashType, {
    {NVMeoFSecureHashType::Invalid, "Invalid"},
    {NVMeoFSecureHashType::SHA256, "SHA256"},
    {NVMeoFSecureHashType::SHA384, "SHA384"},
    {NVMeoFSecureHashType::SHA512, "SHA512"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NVMeoFSecurityTransportType, {
    {NVMeoFSecurityTransportType::Invalid, "Invalid"},
    {NVMeoFSecurityTransportType::TLSv2, "TLSv2"},
    {NVMeoFSecurityTransportType::TLSv3, "TLSv3"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NVMeoFCipherSuiteType, {
    {NVMeoFCipherSuiteType::Invalid, "Invalid"},
    {NVMeoFCipherSuiteType::TLS_AES_128_GCM_SHA256, "TLS_AES_128_GCM_SHA256"},
    {NVMeoFCipherSuiteType::TLS_AES_256_GCM_SHA384, "TLS_AES_256_GCM_SHA384"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NVMeoFDHGroupType, {
    {NVMeoFDHGroupType::Invalid, "Invalid"},
    {NVMeoFDHGroupType::FFDHE2048, "FFDHE2048"},
    {NVMeoFDHGroupType::FFDHE3072, "FFDHE3072"},
    {NVMeoFDHGroupType::FFDHE4096, "FFDHE4096"},
    {NVMeoFDHGroupType::FFDHE6144, "FFDHE6144"},
    {NVMeoFDHGroupType::FFDHE8192, "FFDHE8192"},
});

}
// clang-format on
