#ifndef METRICREPORT_V1
#define METRICREPORT_V1

#include <chrono>
#include "MetricReport_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"

struct MetricReport_v1_Actions
{
    MetricReport_v1_OemActions oem;
};
struct MetricReport_v1_MetricReport
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string reportSequence;
    MetricReport_v1_MetricValue metricValues;
    MetricReport_v1_Actions actions;
    std::chrono::time_point timestamp;
    std::string context;
};
struct MetricReport_v1_MetricValue
{
    std::string metricId;
    std::string metricValue;
    std::chrono::time_point timestamp;
    std::string metricProperty;
    NavigationReference__ metricDefinition;
    Resource_v1_Resource oem;
};
struct MetricReport_v1_OemActions
{
};
#endif
