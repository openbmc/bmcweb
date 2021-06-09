#ifndef ENVIRONMENTMETRICS_V1
#define ENVIRONMENTMETRICS_V1

#include "EnvironmentMetrics_v1.h"
#include "NavigationReference.h"
#include "Resource_v1.h"

struct EnvironmentMetrics_v1_Actions
{
    EnvironmentMetrics_v1_OemActions oem;
};
struct EnvironmentMetrics_v1_EnvironmentMetrics
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    NavigationReference_ temperatureCelsius;
    NavigationReference.humidityPercent;
    NavigationReference_ fanSpeedsPercent;
    NavigationReference_ powerWatts;
    NavigationReference_ energykWh;
    EnvironmentMetrics_v1_Actions actions;
};
struct EnvironmentMetrics_v1_OemActions
{};
#endif
