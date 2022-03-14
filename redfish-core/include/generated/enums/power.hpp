#pragma once
#include <nlohmann/json.hpp>

namespace power
{
// clang-format off

enum class PowerLimitException{
    Invalid,
    NoAction,
    HardPowerOff,
    LogEventOnly,
    Oem,
};

enum class PowerSupplyType{
    Invalid,
    Unknown,
    AC,
    DC,
    ACorDC,
};

enum class LineInputVoltageType{
    Invalid,
    Unknown,
    ACLowLine,
    ACMidLine,
    ACHighLine,
    DCNeg48V,
    DC380V,
    AC120V,
    AC240V,
    AC277V,
    ACandDCWideRange,
    ACWideRange,
    DC240V,
};

enum class InputType{
    Invalid,
    AC,
    DC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerLimitException, {
    {PowerLimitException::Invalid, "Invalid"},
    {PowerLimitException::NoAction, "NoAction"},
    {PowerLimitException::HardPowerOff, "HardPowerOff"},
    {PowerLimitException::LogEventOnly, "LogEventOnly"},
    {PowerLimitException::Oem, "Oem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerSupplyType, {
    {PowerSupplyType::Invalid, "Invalid"},
    {PowerSupplyType::Unknown, "Unknown"},
    {PowerSupplyType::AC, "AC"},
    {PowerSupplyType::DC, "DC"},
    {PowerSupplyType::ACorDC, "ACorDC"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LineInputVoltageType, {
    {LineInputVoltageType::Invalid, "Invalid"},
    {LineInputVoltageType::Unknown, "Unknown"},
    {LineInputVoltageType::ACLowLine, "ACLowLine"},
    {LineInputVoltageType::ACMidLine, "ACMidLine"},
    {LineInputVoltageType::ACHighLine, "ACHighLine"},
    {LineInputVoltageType::DCNeg48V, "DCNeg48V"},
    {LineInputVoltageType::DC380V, "DC380V"},
    {LineInputVoltageType::AC120V, "AC120V"},
    {LineInputVoltageType::AC240V, "AC240V"},
    {LineInputVoltageType::AC277V, "AC277V"},
    {LineInputVoltageType::ACandDCWideRange, "ACandDCWideRange"},
    {LineInputVoltageType::ACWideRange, "ACWideRange"},
    {LineInputVoltageType::DC240V, "DC240V"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(InputType, {
    {InputType::Invalid, "Invalid"},
    {InputType::AC, "AC"},
    {InputType::DC, "DC"},
});

}
// clang-format on
