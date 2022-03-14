#pragma once
#include <nlohmann/json.hpp>

namespace power_supply
{
// clang-format off

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

}
// clang-format on
