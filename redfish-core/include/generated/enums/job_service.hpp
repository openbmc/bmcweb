// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace job_service
{
// clang-format off

enum class ValidationPolicy{
    Invalid,
    Automatic,
    Manual,
    Bypass,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ValidationPolicy, {
    {ValidationPolicy::Invalid, "Invalid"},
    {ValidationPolicy::Automatic, "Automatic"},
    {ValidationPolicy::Manual, "Manual"},
    {ValidationPolicy::Bypass, "Bypass"},
});

}
// clang-format on
