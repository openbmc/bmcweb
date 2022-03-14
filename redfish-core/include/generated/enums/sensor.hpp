#pragma once
#include <nlohmann/json.hpp>

namespace sensor
{
// clang-format off

enum class VoltageType{
    Invalid,
    AC,
    DC,
};

enum class ElectricalContext{
    Invalid,
    Line1,
    Line2,
    Line3,
    Neutral,
    LineToLine,
    Line1ToLine2,
    Line2ToLine3,
    Line3ToLine1,
    LineToNeutral,
    Line1ToNeutral,
    Line2ToNeutral,
    Line3ToNeutral,
    Line1ToNeutralAndL1L2,
    Line2ToNeutralAndL1L2,
    Line2ToNeutralAndL2L3,
    Line3ToNeutralAndL3L1,
    Total,
};

enum class ThresholdActivation{
    Invalid,
    Increasing,
    Decreasing,
    Either,
};

enum class ReadingType{
    Invalid,
    Temperature,
    Humidity,
    Power,
    EnergykWh,
    EnergyJoules,
    EnergyWh,
    ChargeAh,
    Voltage,
    Current,
    Frequency,
    Pressure,
    PressurekPa,
    LiquidLevel,
    Rotational,
    AirFlow,
    LiquidFlow,
    Barometric,
    Altitude,
    Percent,
    AbsoluteHumidity,
};

enum class ImplementationType{
    Invalid,
    PhysicalSensor,
    Synthesized,
    Reported,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VoltageType, {
    {VoltageType::Invalid, "Invalid"},
    {VoltageType::AC, "AC"},
    {VoltageType::DC, "DC"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ElectricalContext, {
    {ElectricalContext::Invalid, "Invalid"},
    {ElectricalContext::Line1, "Line1"},
    {ElectricalContext::Line2, "Line2"},
    {ElectricalContext::Line3, "Line3"},
    {ElectricalContext::Neutral, "Neutral"},
    {ElectricalContext::LineToLine, "LineToLine"},
    {ElectricalContext::Line1ToLine2, "Line1ToLine2"},
    {ElectricalContext::Line2ToLine3, "Line2ToLine3"},
    {ElectricalContext::Line3ToLine1, "Line3ToLine1"},
    {ElectricalContext::LineToNeutral, "LineToNeutral"},
    {ElectricalContext::Line1ToNeutral, "Line1ToNeutral"},
    {ElectricalContext::Line2ToNeutral, "Line2ToNeutral"},
    {ElectricalContext::Line3ToNeutral, "Line3ToNeutral"},
    {ElectricalContext::Line1ToNeutralAndL1L2, "Line1ToNeutralAndL1L2"},
    {ElectricalContext::Line2ToNeutralAndL1L2, "Line2ToNeutralAndL1L2"},
    {ElectricalContext::Line2ToNeutralAndL2L3, "Line2ToNeutralAndL2L3"},
    {ElectricalContext::Line3ToNeutralAndL3L1, "Line3ToNeutralAndL3L1"},
    {ElectricalContext::Total, "Total"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ThresholdActivation, {
    {ThresholdActivation::Invalid, "Invalid"},
    {ThresholdActivation::Increasing, "Increasing"},
    {ThresholdActivation::Decreasing, "Decreasing"},
    {ThresholdActivation::Either, "Either"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ReadingType, {
    {ReadingType::Invalid, "Invalid"},
    {ReadingType::Temperature, "Temperature"},
    {ReadingType::Humidity, "Humidity"},
    {ReadingType::Power, "Power"},
    {ReadingType::EnergykWh, "EnergykWh"},
    {ReadingType::EnergyJoules, "EnergyJoules"},
    {ReadingType::EnergyWh, "EnergyWh"},
    {ReadingType::ChargeAh, "ChargeAh"},
    {ReadingType::Voltage, "Voltage"},
    {ReadingType::Current, "Current"},
    {ReadingType::Frequency, "Frequency"},
    {ReadingType::Pressure, "Pressure"},
    {ReadingType::PressurekPa, "PressurekPa"},
    {ReadingType::LiquidLevel, "LiquidLevel"},
    {ReadingType::Rotational, "Rotational"},
    {ReadingType::AirFlow, "AirFlow"},
    {ReadingType::LiquidFlow, "LiquidFlow"},
    {ReadingType::Barometric, "Barometric"},
    {ReadingType::Altitude, "Altitude"},
    {ReadingType::Percent, "Percent"},
    {ReadingType::AbsoluteHumidity, "AbsoluteHumidity"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ImplementationType, {
    {ImplementationType::Invalid, "Invalid"},
    {ImplementationType::PhysicalSensor, "PhysicalSensor"},
    {ImplementationType::Synthesized, "Synthesized"},
    {ImplementationType::Reported, "Reported"},
});

}
// clang-format on
