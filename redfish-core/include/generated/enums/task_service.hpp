// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace task_service
{
enum class OverWritePolicy{
    Invalid,
    Manual,
    Oldest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OverWritePolicy, {
    {OverWritePolicy::Invalid, "Invalid"},
    {OverWritePolicy::Manual, "Manual"},
    {OverWritePolicy::Oldest, "Oldest"},
});

// clang-format on
}
