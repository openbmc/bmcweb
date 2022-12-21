#pragma once
#include <nlohmann/json.hpp>

namespace key
{
// clang-format off

enum class KeyType{
    Invalid,
    NVMeoF,
    SSH,
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

NLOHMANN_JSON_SERIALIZE_ENUM(KeyType, {
    {KeyType::Invalid, "Invalid"},
    {KeyType::NVMeoF, "NVMeoF"},
    {KeyType::SSH, "SSH"},
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

}
// clang-format on
