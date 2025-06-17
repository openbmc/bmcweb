// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace telemetry_data
{
// clang-format off

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
