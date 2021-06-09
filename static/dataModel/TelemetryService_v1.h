#ifndef TELEMETRYSERVICE_V1
#define TELEMETRYSERVICE_V1

#include <chrono>
#include "MetricDefinitionCollection_v1.h"
#include "MetricReportCollection_v1.h"
#include "MetricReportDefinitionCollection_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "TelemetryService_v1.h"
#include "TriggersCollection_v1.h"

enum class TelemetryService_v1_CollectionFunction {
    Average,
    Maximum,
    Minimum,
    Summation,
};
struct TelemetryService_v1_Actions
{
    TelemetryService_v1_OemActions oem;
};
struct TelemetryService_v1_MetricValue
{
    std::string metricId;
    std::string metricValue;
    std::chrono::time_point timestamp;
    std::string metricProperty;
    NavigationReference__ metricDefinition;
};
struct TelemetryService_v1_OemActions
{
};
struct TelemetryService_v1_TelemetryService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    int64_t maxReports;
    std::chrono::milliseconds minCollectionInterval;
    TelemetryService_v1_CollectionFunction supportedCollectionFunctions;
    MetricDefinitionCollection_v1_MetricDefinitionCollection metricDefinitions;
    MetricReportDefinitionCollection_v1_MetricReportDefinitionCollection metricReportDefinitions;
    MetricReportCollection_v1_MetricReportCollection metricReports;
    TriggersCollection_v1_TriggersCollection triggers;
    NavigationReference__ logService;
    TelemetryService_v1_Actions actions;
    bool serviceEnabled;
};
#endif
