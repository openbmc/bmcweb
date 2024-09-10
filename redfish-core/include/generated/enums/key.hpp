// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace key
{
// clang-format off

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
