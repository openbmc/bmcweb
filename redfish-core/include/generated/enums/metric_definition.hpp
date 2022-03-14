#pragma once
#include <nlohmann/json.hpp>

namespace metric_definition
{
// clang-format off

enum class MetricType{
    Invalid,
    Numeric,
    Discrete,
    Gauge,
    Counter,
    Countdown,
    String,
};

enum class ImplementationType{
    Invalid,
    PhysicalSensor,
    Calculated,
    Synthesized,
    DigitalMeter,
};

enum class MetricDataType{
    Invalid,
    Boolean,
    DateTime,
    Decimal,
    Integer,
    String,
    Enumeration,
};

enum class Calculable{
    Invalid,
    NonCalculatable,
    Summable,
    NonSummable,
};

enum class CalculationAlgorithmEnum{
    Invalid,
    Average,
    Maximum,
    Minimum,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricType, {
    {MetricType::Invalid, "Invalid"},
    {MetricType::Numeric, "Numeric"},
    {MetricType::Discrete, "Discrete"},
    {MetricType::Gauge, "Gauge"},
    {MetricType::Counter, "Counter"},
    {MetricType::Countdown, "Countdown"},
    {MetricType::String, "String"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ImplementationType, {
    {ImplementationType::Invalid, "Invalid"},
    {ImplementationType::PhysicalSensor, "PhysicalSensor"},
    {ImplementationType::Calculated, "Calculated"},
    {ImplementationType::Synthesized, "Synthesized"},
    {ImplementationType::DigitalMeter, "DigitalMeter"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MetricDataType, {
    {MetricDataType::Invalid, "Invalid"},
    {MetricDataType::Boolean, "Boolean"},
    {MetricDataType::DateTime, "DateTime"},
    {MetricDataType::Decimal, "Decimal"},
    {MetricDataType::Integer, "Integer"},
    {MetricDataType::String, "String"},
    {MetricDataType::Enumeration, "Enumeration"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Calculable, {
    {Calculable::Invalid, "Invalid"},
    {Calculable::NonCalculatable, "NonCalculatable"},
    {Calculable::Summable, "Summable"},
    {Calculable::NonSummable, "NonSummable"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CalculationAlgorithmEnum, {
    {CalculationAlgorithmEnum::Invalid, "Invalid"},
    {CalculationAlgorithmEnum::Average, "Average"},
    {CalculationAlgorithmEnum::Maximum, "Maximum"},
    {CalculationAlgorithmEnum::Minimum, "Minimum"},
    {CalculationAlgorithmEnum::OEM, "OEM"},
});

}
// clang-format on
