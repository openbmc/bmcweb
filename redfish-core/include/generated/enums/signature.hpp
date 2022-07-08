#pragma once
#include <nlohmann/json.hpp>

namespace signature
{
// clang-format off

enum class SignatureTypeRegistry{
    Invalid,
    UEFI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SignatureTypeRegistry, {
    {SignatureTypeRegistry::Invalid, "Invalid"},
    {SignatureTypeRegistry::UEFI, "UEFI"},
});

}
// clang-format on
