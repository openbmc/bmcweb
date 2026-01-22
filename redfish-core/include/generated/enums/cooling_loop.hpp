// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace cooling_loop
{
// clang-format off

enum class CoolantType{
    Invalid,
    Water,
    Hydrocarbon,
    Fluorocarbon,
    Dielectric,
};

enum class CoolingLoopType{
    Invalid,
    FWS,
    TCS,
    RowTCS,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CoolantType, {
    {CoolantType::Invalid, "Invalid"},
    {CoolantType::Water, "Water"},
    {CoolantType::Hydrocarbon, "Hydrocarbon"},
    {CoolantType::Fluorocarbon, "Fluorocarbon"},
    {CoolantType::Dielectric, "Dielectric"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CoolingLoopType, {
    {CoolingLoopType::Invalid, "Invalid"},
    {CoolingLoopType::FWS, "FWS"},
    {CoolingLoopType::TCS, "TCS"},
    {CoolingLoopType::RowTCS, "RowTCS"},
});

// clang-format on
}
