// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace thermal
{
enum class ReadingUnits{
    Invalid,
    RPM,
    Percent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ReadingUnits, {
    {ReadingUnits::Invalid, "Invalid"},
    {ReadingUnits::RPM, "RPM"},
    {ReadingUnits::Percent, "Percent"},
});

// clang-format on
}
