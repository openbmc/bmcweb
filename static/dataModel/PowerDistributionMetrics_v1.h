#ifndef POWERDISTRIBUTIONMETRICS_V1
#define POWERDISTRIBUTIONMETRICS_V1

#include "NavigationReference__.h"
#include "PowerDistributionMetrics_v1.h"
#include "Resource_v1.h"

struct PowerDistributionMetrics_v1_Actions
{
    PowerDistributionMetrics_v1_OemActions oem;
};
struct PowerDistributionMetrics_v1_OemActions
{
};
struct PowerDistributionMetrics_v1_PowerDistributionMetrics
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    NavigationReference__ powerWatts;
    NavigationReference__ energykWh;
    PowerDistributionMetrics_v1_Actions actions;
};
#endif
