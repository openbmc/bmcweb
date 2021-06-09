#ifndef NETWORKADAPTERMETRICS_V1
#define NETWORKADAPTERMETRICS_V1

#include "NetworkAdapterMetrics_v1.h"
#include "Resource_v1.h"

struct NetworkAdapterMetricsV1OemActions
{};
struct NetworkAdapterMetricsV1Actions
{
    NetworkAdapterMetricsV1OemActions oem;
};
struct NetworkAdapterMetricsV1NetworkAdapterMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    double hostBusRXPercent;
    double hostBusTXPercent;
    double cPUCorePercent;
    int64_t nCSIRXFrames;
    int64_t nCSITXFrames;
    int64_t nCSIRXBytes;
    int64_t nCSITXBytes;
    int64_t rXBytes;
    int64_t rXMulticastFrames;
    int64_t rXUnicastFrames;
    int64_t tXBytes;
    int64_t tXMulticastFrames;
    int64_t tXUnicastFrames;
    NetworkAdapterMetricsV1Actions actions;
};
#endif
