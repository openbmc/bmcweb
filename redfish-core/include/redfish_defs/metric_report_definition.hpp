#pragma once
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

NLOHMANN_JSON_SERIALIZE_ENUM(MetricReportDefinitionType, { //NOLINT
    {MetricReportDefinitionType::Invalid, "Invalid"},
    {MetricReportDefinitionType::Periodic, "Periodic"},
    {MetricReportDefinitionType::OnChange, "OnChange"},
    {MetricReportDefinitionType::OnRequest, "OnRequest"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ReportActionsEnum, { //NOLINT
    {ReportActionsEnum::Invalid, "Invalid"},
    {ReportActionsEnum::LogToMetricReportsCollection, "LogToMetricReportsCollection"},
    {ReportActionsEnum::RedfishEvent, "RedfishEvent"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ReportUpdatesEnum, { //NOLINT
    {ReportUpdatesEnum::Invalid, "Invalid"},
    {ReportUpdatesEnum::Overwrite, "Overwrite"},
    {ReportUpdatesEnum::AppendWrapsWhenFull, "AppendWrapsWhenFull"},
    {ReportUpdatesEnum::AppendStopsWhenFull, "AppendStopsWhenFull"},
    {ReportUpdatesEnum::NewReport, "NewReport"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CalculationAlgorithmEnum, { //NOLINT
    {CalculationAlgorithmEnum::Invalid, "Invalid"},
    {CalculationAlgorithmEnum::Average, "Average"},
    {CalculationAlgorithmEnum::Maximum, "Maximum"},
    {CalculationAlgorithmEnum::Minimum, "Minimum"},
    {CalculationAlgorithmEnum::Summation, "Summation"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CollectionTimeScope, { //NOLINT
    {CollectionTimeScope::Invalid, "Invalid"},
    {CollectionTimeScope::Point, "Point"},
    {CollectionTimeScope::Interval, "Interval"},
    {CollectionTimeScope::StartupInterval, "StartupInterval"},
});

}
// clang-format on
