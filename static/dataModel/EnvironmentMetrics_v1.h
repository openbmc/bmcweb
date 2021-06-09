#ifndef ENVIRONMENTMETRICS_V1
#define ENVIRONMENTMETRICS_V1

#include "EnvironmentMetrics_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

struct EnvironmentMetricsV1OemActions
{};
struct EnvironmentMetricsV1Actions
{
    EnvironmentMetricsV1OemActions oem;
};
struct EnvironmentMetricsV1EnvironmentMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    NavigationReference_ temperatureCelsius;
    NavigationReference_ humidityPercent;
    NavigationReference_ fanSpeedsPercent;
    NavigationReference_ powerWatts;
    NavigationReference_ energykWh;
    EnvironmentMetricsV1Actions actions;
};
#endif
