// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace telemetry_data
{
enum class TelemetryDataTypes{
    Invalid,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TelemetryDataTypes, {
    {TelemetryDataTypes::Invalid, "Invalid"},
    {TelemetryDataTypes::OEM, "OEM"},
});

}
// clang-format on
