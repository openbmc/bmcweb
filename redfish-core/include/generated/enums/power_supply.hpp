#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(PowerSupplyType,

    Invalid,
    AC,
    DC,
    ACorDC,
    DCRegulator,
);

NLOHMANN_JSON_SERIALIZE_ENUM(LineStatus, {
    {LineStatus::Invalid, "Invalid"},
    {LineStatus::Normal, "Normal"},
    {LineStatus::LossOfInput, "LossOfInput"},
    {LineStatus::OutOfRange, "OutOfRange"},
});

BOOST_DESCRIBE_ENUM(LineStatus,

    Invalid,
    Normal,
    LossOfInput,
    OutOfRange,
);

}
// clang-format on
