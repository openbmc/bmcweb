#pragma once
#include <boost/describe/enum.hpp>
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

enum class TriggerActionMessage{
    Invalid,
    Telemetry,
    DriveMediaLife,
    ConnectionSpeed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricTypeEnum, {
    {MetricTypeEnum::Invalid, "Invalid"},
    {MetricTypeEnum::Numeric, "Numeric"},
    {MetricTypeEnum::Discrete, "Discrete"},
});

BOOST_DESCRIBE_ENUM(MetricTypeEnum,

    Invalid,
    Numeric,
    Discrete,
);

NLOHMANN_JSON_SERIALIZE_ENUM(TriggerActionEnum, {
    {TriggerActionEnum::Invalid, "Invalid"},
    {TriggerActionEnum::LogToLogService, "LogToLogService"},
    {TriggerActionEnum::RedfishEvent, "RedfishEvent"},
    {TriggerActionEnum::RedfishMetricReport, "RedfishMetricReport"},
});

BOOST_DESCRIBE_ENUM(TriggerActionEnum,

    Invalid,
    LogToLogService,
    RedfishEvent,
    RedfishMetricReport,
);

NLOHMANN_JSON_SERIALIZE_ENUM(DiscreteTriggerConditionEnum, {
    {DiscreteTriggerConditionEnum::Invalid, "Invalid"},
    {DiscreteTriggerConditionEnum::Specified, "Specified"},
    {DiscreteTriggerConditionEnum::Changed, "Changed"},
});

BOOST_DESCRIBE_ENUM(DiscreteTriggerConditionEnum,

    Invalid,
    Specified,
    Changed,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ThresholdActivation, {
    {ThresholdActivation::Invalid, "Invalid"},
    {ThresholdActivation::Increasing, "Increasing"},
    {ThresholdActivation::Decreasing, "Decreasing"},
    {ThresholdActivation::Either, "Either"},
    {ThresholdActivation::Disabled, "Disabled"},
});

BOOST_DESCRIBE_ENUM(ThresholdActivation,

    Invalid,
    Increasing,
    Decreasing,
    Either,
    Disabled,
);

NLOHMANN_JSON_SERIALIZE_ENUM(DirectionOfCrossingEnum, {
    {DirectionOfCrossingEnum::Invalid, "Invalid"},
    {DirectionOfCrossingEnum::Increasing, "Increasing"},
    {DirectionOfCrossingEnum::Decreasing, "Decreasing"},
});

BOOST_DESCRIBE_ENUM(DirectionOfCrossingEnum,

    Invalid,
    Increasing,
    Decreasing,
);

NLOHMANN_JSON_SERIALIZE_ENUM(TriggerActionMessage, {
    {TriggerActionMessage::Invalid, "Invalid"},
    {TriggerActionMessage::Telemetry, "Telemetry"},
    {TriggerActionMessage::DriveMediaLife, "DriveMediaLife"},
    {TriggerActionMessage::ConnectionSpeed, "ConnectionSpeed"},
});

BOOST_DESCRIBE_ENUM(TriggerActionMessage,

    Invalid,
    Telemetry,
    DriveMediaLife,
    ConnectionSpeed,
);

}
// clang-format on
