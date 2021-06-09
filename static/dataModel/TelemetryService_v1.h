#ifndef TELEMETRYSERVICE_V1
#define TELEMETRYSERVICE_V1

#include "MetricDefinitionCollection_v1.h"
#include "MetricReportCollection_v1.h"
#include "MetricReportDefinitionCollection_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "TelemetryService_v1.h"
#include "TriggersCollection_v1.h"

#include <chrono>

enum class TelemetryServiceV1CollectionFunction
{
    Average,
    Maximum,
    Minimum,
    Summation,
};
struct TelemetryServiceV1OemActions
{};
struct TelemetryServiceV1Actions
{
    TelemetryServiceV1OemActions oem;
};
struct TelemetryServiceV1MetricValue
{
    std::string metricId;
    std::string metricValue;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string metricProperty;
    NavigationReference_ metricDefinition;
};
struct TelemetryServiceV1TelemetryService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    int64_t maxReports;
    std::chrono::milliseconds minCollectionInterval;
    TelemetryServiceV1CollectionFunction supportedCollectionFunctions;
    MetricDefinitionCollectionV1MetricDefinitionCollection metricDefinitions;
    MetricReportDefinitionCollectionV1MetricReportDefinitionCollection
        metricReportDefinitions;
    MetricReportCollectionV1MetricReportCollection metricReports;
    TriggersCollectionV1TriggersCollection triggers;
    NavigationReference_ logService;
    TelemetryServiceV1Actions actions;
    bool serviceEnabled;
};
#endif
