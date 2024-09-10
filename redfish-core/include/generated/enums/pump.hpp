// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace pump
{
// clang-format off

enum class PumpType{
    Invalid,
    Liquid,
    Compressor,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PumpType, {
    {PumpType::Invalid, "Invalid"},
    {PumpType::Liquid, "Liquid"},
    {PumpType::Compressor, "Compressor"},
});

}
// clang-format on
