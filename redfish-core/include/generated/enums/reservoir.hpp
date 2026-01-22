// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace reservoir
{
enum class ReservoirType{
    Invalid,
    Reserve,
    Overflow,
    Inline,
    Immersion,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ReservoirType, {
    {ReservoirType::Invalid, "Invalid"},
    {ReservoirType::Reserve, "Reserve"},
    {ReservoirType::Overflow, "Overflow"},
    {ReservoirType::Inline, "Inline"},
    {ReservoirType::Immersion, "Immersion"},
});

// clang-format on
}
