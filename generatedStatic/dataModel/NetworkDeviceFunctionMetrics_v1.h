#ifndef NETWORKDEVICEFUNCTIONMETRICS_V1
#define NETWORKDEVICEFUNCTIONMETRICS_V1

#include "NetworkDeviceFunctionMetrics_v1.h"
#include "Resource_v1.h"

struct NetworkDeviceFunctionMetricsV1OemActions
{};
struct NetworkDeviceFunctionMetricsV1Actions
{
    NetworkDeviceFunctionMetricsV1OemActions oem;
};
struct NetworkDeviceFunctionMetricsV1Ethernet
{
    int64_t numOffloadedIPv4Conns;
    int64_t numOffloadedIPv6Conns;
};
struct NetworkDeviceFunctionMetricsV1NetworkDeviceFunctionMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    NetworkDeviceFunctionMetricsV1Ethernet ethernet;
    double tXAvgQueueDepthPercent;
    double rXAvgQueueDepthPercent;
    int64_t rXFrames;
    int64_t rXBytes;
    int64_t rXUnicastFrames;
    int64_t rXMulticastFrames;
    int64_t tXFrames;
    int64_t tXBytes;
    int64_t tXUnicastFrames;
    int64_t tXMulticastFrames;
    bool tXQueuesEmpty;
    bool rXQueuesEmpty;
    int64_t tXQueuesFull;
    int64_t rXQueuesFull;
    NetworkDeviceFunctionMetricsV1Actions actions;
};
#endif
