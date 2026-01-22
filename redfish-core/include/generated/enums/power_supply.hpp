// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace power_supply
{
enum class PowerSupplyType{
    Invalid,
    AC,
    DC,
    ACorDC,
    DCRegulator,
};

enum class LineStatus{
    Invalid,
    Normal,
    LossOfInput,
    OutOfRange,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerSupplyType, {
    {PowerSupplyType::Invalid, "Invalid"},
    {PowerSupplyType::AC, "AC"},
    {PowerSupplyType::DC, "DC"},
    {PowerSupplyType::ACorDC, "ACorDC"},
    {PowerSupplyType::DCRegulator, "DCRegulator"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LineStatus, {
    {LineStatus::Invalid, "Invalid"},
    {LineStatus::Normal, "Normal"},
    {LineStatus::LossOfInput, "LossOfInput"},
    {LineStatus::OutOfRange, "OutOfRange"},
});

// clang-format on
}
