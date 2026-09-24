// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace policy_service
{
// clang-format off

enum class OperatingMode{
    Invalid,
    Disabled,
    AlertOnly,
    Enabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OperatingMode, {
    {OperatingMode::Invalid, "Invalid"},
    {OperatingMode::Disabled, "Disabled"},
    {OperatingMode::AlertOnly, "AlertOnly"},
    {OperatingMode::Enabled, "Enabled"},
});

// clang-format on
} // namespace policy_service
