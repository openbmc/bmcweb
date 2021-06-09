#ifndef POWERDISTRIBUTIONMETRICS_V1
#define POWERDISTRIBUTIONMETRICS_V1

#include "NavigationReferenceRedfish.h"
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
    NavigationReferenceRedfish powerWatts;
    NavigationReferenceRedfish energykWh;
    PowerDistributionMetricsV1Actions actions;
    NavigationReferenceRedfish temperatureCelsius;
    NavigationReferenceRedfish humidityPercent;
};
#endif
