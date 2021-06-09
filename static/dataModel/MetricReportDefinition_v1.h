#ifndef METRICREPORTDEFINITION_V1
#define METRICREPORTDEFINITION_V1

#include <chrono>
#include "MetricReportDefinition_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"

enum class MetricReportDefinition_v1_CalculationAlgorithmEnum {
    Average,
    Maximum,
    Minimum,
    Summation,
};
enum class MetricReportDefinition_v1_CollectionTimeScope {
    Point,
    Interval,
    StartupInterval,
};
enum class MetricReportDefinition_v1_MetricReportDefinitionType {
    Periodic,
    OnChange,
    OnRequest,
};
enum class MetricReportDefinition_v1_ReportActionsEnum {
    LogToMetricReportsCollection,
    RedfishEvent,
};
enum class MetricReportDefinition_v1_ReportUpdatesEnum {
    Overwrite,
    AppendWrapsWhenFull,
    AppendStopsWhenFull,
    NewReport,
};
struct MetricReportDefinition_v1_Actions
{
    MetricReportDefinition_v1_OemActions oem;
};
struct MetricReportDefinition_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ triggers;
};
struct MetricReportDefinition_v1_Metric
{
    std::string metricId;
    std::string metricProperties;
    MetricReportDefinition_v1_CalculationAlgorithmEnum collectionFunction;
    std::chrono::milliseconds collectionDuration;
    MetricReportDefinition_v1_CollectionTimeScope collectionTimeScope;
};
struct MetricReportDefinition_v1_MetricReportDefinition
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    MetricReportDefinition_v1_MetricReportDefinitionType metricReportDefinitionType;
    Schedule_v1_Schedule schedule;
    MetricReportDefinition_v1_ReportActionsEnum reportActions;
    MetricReportDefinition_v1_ReportUpdatesEnum reportUpdates;
    int64_t appendLimit;
    Resource_v1_Resource status;
    MetricReportDefinition_v1_Wildcard wildcards;
    std::string metricProperties;
    MetricReportDefinition_v1_Metric metrics;
    NavigationReference__ metricReport;
    MetricReportDefinition_v1_Actions actions;
    bool suppressRepeatedMetricValue;
    std::chrono::milliseconds metricReportHeartbeatInterval;
    bool metricReportDefinitionEnabled;
    MetricReportDefinition_v1_Links links;
    std::chrono::milliseconds reportTimespan;
};
struct MetricReportDefinition_v1_OemActions
{
};
struct MetricReportDefinition_v1_Wildcard
{
    std::string name;
    std::string keys;
    std::string values;
};
#endif
