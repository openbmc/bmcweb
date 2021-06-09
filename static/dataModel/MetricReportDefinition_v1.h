#ifndef METRICREPORTDEFINITION_V1
#define METRICREPORTDEFINITION_V1

#include "MetricReportDefinition_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"

#include <chrono>

enum class MetricReportDefinitionV1CalculationAlgorithmEnum
{
    Average,
    Maximum,
    Minimum,
    Summation,
};
enum class MetricReportDefinitionV1CollectionTimeScope
{
    Point,
    Interval,
    StartupInterval,
};
enum class MetricReportDefinitionV1MetricReportDefinitionType
{
    Periodic,
    OnChange,
    OnRequest,
};
enum class MetricReportDefinitionV1ReportActionsEnum
{
    LogToMetricReportsCollection,
    RedfishEvent,
};
enum class MetricReportDefinitionV1ReportUpdatesEnum
{
    Overwrite,
    AppendWrapsWhenFull,
    AppendStopsWhenFull,
    NewReport,
};
struct MetricReportDefinitionV1OemActions
{};
struct MetricReportDefinitionV1Actions
{
    MetricReportDefinitionV1OemActions oem;
};
struct MetricReportDefinitionV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ triggers;
};
struct MetricReportDefinitionV1Metric
{
    std::string metricId;
    std::string metricProperties;
    MetricReportDefinitionV1CalculationAlgorithmEnum collectionFunction;
    std::chrono::milliseconds collectionDuration;
    MetricReportDefinitionV1CollectionTimeScope collectionTimeScope;
    ResourceV1Resource oem;
};
struct MetricReportDefinitionV1Wildcard
{
    std::string name;
    std::string keys;
    std::string values;
};
struct MetricReportDefinitionV1MetricReportDefinition
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    MetricReportDefinitionV1MetricReportDefinitionType
        metricReportDefinitionType;
    ScheduleV1Schedule schedule;
    MetricReportDefinitionV1ReportActionsEnum reportActions;
    MetricReportDefinitionV1ReportUpdatesEnum reportUpdates;
    int64_t appendLimit;
    ResourceV1Resource status;
    MetricReportDefinitionV1Wildcard wildcards;
    std::string metricProperties;
    MetricReportDefinitionV1Metric metrics;
    NavigationReference_ metricReport;
    MetricReportDefinitionV1Actions actions;
    bool suppressRepeatedMetricValue;
    std::chrono::milliseconds metricReportHeartbeatInterval;
    bool metricReportDefinitionEnabled;
    MetricReportDefinitionV1Links links;
    std::chrono::milliseconds reportTimespan;
};
#endif
