// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace signature
{
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
