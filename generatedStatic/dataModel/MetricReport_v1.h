#ifndef METRICREPORT_V1
#define METRICREPORT_V1

#include "MetricReport_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

#include <chrono>

struct MetricReportV1OemActions
{};
struct MetricReportV1Actions
{
    MetricReportV1OemActions oem;
};
struct MetricReportV1MetricValue
{
    std::string metricId;
    std::string metricValue;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string metricProperty;
    NavigationReferenceRedfish metricDefinition;
    ResourceV1Resource oem;
};
struct MetricReportV1MetricReport
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string reportSequence;
    MetricReportV1MetricValue metricValues;
    MetricReportV1Actions actions;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    std::string context;
};
#endif
