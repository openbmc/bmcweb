#ifndef POWERDISTRIBUTIONMETRICS_V1
#define POWERDISTRIBUTIONMETRICS_V1

#include "NavigationReference_.h"
#include "PowerDistributionMetrics_v1.h"
#include "Resource_v1.h"

struct PowerDistributionMetricsV1OemActions
{};
struct PowerDistributionMetricsV1Actions
{
    PowerDistributionMetricsV1OemActions oem;
};
struct PowerDistributionMetricsV1PowerDistributionMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    NavigationReference_ powerWatts;
    NavigationReference_ energykWh;
    PowerDistributionMetricsV1Actions actions;
    NavigationReference_ temperatureCelsius;
    NavigationReference_ humidityPercent;
};
#endif
