// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace open_bmc_manager
{
// clang-format off

enum class HysteresisEvaluation{
    Invalid,
    Input,
    Setpoint,
};

NLOHMANN_JSON_SERIALIZE_ENUM(HysteresisEvaluation, {
    {HysteresisEvaluation::Invalid, "Invalid"},
    {HysteresisEvaluation::Input, "Input"},
    {HysteresisEvaluation::Setpoint, "Setpoint"},
});

// clang-format on
} // namespace open_bmc_manager
