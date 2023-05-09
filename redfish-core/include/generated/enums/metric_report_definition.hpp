#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace metric_report_definition
{
// clang-format off

enum class MetricReportDefinitionType{
    Invalid,
    Periodic,
    OnChange,
    OnRequest,
};

enum class ReportActionsEnum{
    Invalid,
    LogToMetricReportsCollection,
    RedfishEvent,
};

enum class ReportUpdatesEnum{
    Invalid,
    Overwrite,
    AppendWrapsWhenFull,
    AppendStopsWhenFull,
    NewReport,
};

enum class CalculationAlgorithmEnum{
    Invalid,
    Average,
    Maximum,
    Minimum,
    Summation,
};

enum class CollectionTimeScope{
    Invalid,
    Point,
    Interval,
    StartupInterval,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MetricReportDefinitionType, {
    {MetricReportDefinitionType::Invalid, "Invalid"},
    {MetricReportDefinitionType::Periodic, "Periodic"},
    {MetricReportDefinitionType::OnChange, "OnChange"},
    {MetricReportDefinitionType::OnRequest, "OnRequest"},
});

BOOST_DESCRIBE_ENUM(MetricReportDefinitionType,

    Invalid,
    Periodic,
    OnChange,
    OnRequest,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ReportActionsEnum, {
    {ReportActionsEnum::Invalid, "Invalid"},
    {ReportActionsEnum::LogToMetricReportsCollection, "LogToMetricReportsCollection"},
    {ReportActionsEnum::RedfishEvent, "RedfishEvent"},
});

BOOST_DESCRIBE_ENUM(ReportActionsEnum,

    Invalid,
    LogToMetricReportsCollection,
    RedfishEvent,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ReportUpdatesEnum, {
    {ReportUpdatesEnum::Invalid, "Invalid"},
    {ReportUpdatesEnum::Overwrite, "Overwrite"},
    {ReportUpdatesEnum::AppendWrapsWhenFull, "AppendWrapsWhenFull"},
    {ReportUpdatesEnum::AppendStopsWhenFull, "AppendStopsWhenFull"},
    {ReportUpdatesEnum::NewReport, "NewReport"},
});

BOOST_DESCRIBE_ENUM(ReportUpdatesEnum,

    Invalid,
    Overwrite,
    AppendWrapsWhenFull,
    AppendStopsWhenFull,
    NewReport,
);

NLOHMANN_JSON_SERIALIZE_ENUM(CalculationAlgorithmEnum, {
    {CalculationAlgorithmEnum::Invalid, "Invalid"},
    {CalculationAlgorithmEnum::Average, "Average"},
    {CalculationAlgorithmEnum::Maximum, "Maximum"},
    {CalculationAlgorithmEnum::Minimum, "Minimum"},
    {CalculationAlgorithmEnum::Summation, "Summation"},
});

BOOST_DESCRIBE_ENUM(CalculationAlgorithmEnum,

    Invalid,
    Average,
    Maximum,
    Minimum,
    Summation,
);

NLOHMANN_JSON_SERIALIZE_ENUM(CollectionTimeScope, {
    {CollectionTimeScope::Invalid, "Invalid"},
    {CollectionTimeScope::Point, "Point"},
    {CollectionTimeScope::Interval, "Interval"},
    {CollectionTimeScope::StartupInterval, "StartupInterval"},
});

BOOST_DESCRIBE_ENUM(CollectionTimeScope,

    Invalid,
    Point,
    Interval,
    StartupInterval,
);

}
// clang-format on
