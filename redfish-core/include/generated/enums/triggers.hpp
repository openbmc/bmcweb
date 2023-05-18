#pragma once
#include <nlohmann/json.hpp>

namespace triggers
{
// clang-format off

enum class MetricTypeEnum{
    Invalid,
    Numeric,
    Discrete,
};

enum class TriggerActionEnum{
    Invalid,
    LogToLogService,
    RedfishEvent,
    RedfishMetricReport,
};

enum class DiscreteTriggerConditionEnum{
    Invalid,
    Specified,
    Changed,
};

enum class ThresholdActivation{
    Invalid,
    Increasing,
    Decreasing,
    Either,
    Disabled,
};

enum class DirectionOfCrossingEnum{
    Invalid,
    Increasing,
    Decreasing,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricTypeEnum, {
    {MetricTypeEnum::Invalid, "Invalid"},
    {MetricTypeEnum::Numeric, "Numeric"},
    {MetricTypeEnum::Discrete, "Discrete"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(TriggerActionEnum, {
    {TriggerActionEnum::Invalid, "Invalid"},
    {TriggerActionEnum::LogToLogService, "LogToLogService"},
    {TriggerActionEnum::RedfishEvent, "RedfishEvent"},
    {TriggerActionEnum::RedfishMetricReport, "RedfishMetricReport"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DiscreteTriggerConditionEnum, {
    {DiscreteTriggerConditionEnum::Invalid, "Invalid"},
    {DiscreteTriggerConditionEnum::Specified, "Specified"},
    {DiscreteTriggerConditionEnum::Changed, "Changed"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ThresholdActivation, {
    {ThresholdActivation::Invalid, "Invalid"},
    {ThresholdActivation::Increasing, "Increasing"},
    {ThresholdActivation::Decreasing, "Decreasing"},
    {ThresholdActivation::Either, "Either"},
    {ThresholdActivation::Disabled, "Disabled"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DirectionOfCrossingEnum, {
    {DirectionOfCrossingEnum::Invalid, "Invalid"},
    {DirectionOfCrossingEnum::Increasing, "Increasing"},
    {DirectionOfCrossingEnum::Decreasing, "Decreasing"},
});

}
// clang-format on
