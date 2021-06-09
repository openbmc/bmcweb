#ifndef PROCESSORMETRICS_V1
#define PROCESSORMETRICS_V1

#include "ProcessorMetrics_v1.h"
#include "Resource_v1.h"

struct ProcessorMetricsV1OemActions
{};
struct ProcessorMetricsV1Actions
{
    ProcessorMetricsV1OemActions oem;
};
struct ProcessorMetricsV1CacheMetrics
{
    std::string level;
    double cacheMiss;
    double hitRatio;
    double cacheMissesPerInstruction;
    int64_t occupancyBytes;
    double occupancyPercent;
};
struct ProcessorMetricsV1CurrentPeriod
{
    int64_t correctableECCErrorCount;
    int64_t uncorrectableECCErrorCount;
};
struct ProcessorMetricsV1LifeTime
{
    int64_t correctableECCErrorCount;
    int64_t uncorrectableECCErrorCount;
};
struct ProcessorMetricsV1CacheMetricsTotal
{
    ProcessorMetricsV1CurrentPeriod currentPeriod;
    ProcessorMetricsV1LifeTime lifeTime;
};
struct ProcessorMetricsV1CStateResidency
{
    std::string level;
    double residencyPercent;
};
struct ProcessorMetricsV1CoreMetrics
{
    std::string coreId;
    double instructionsPerCycle;
    double unhaltedCycles;
    double memoryStallCount;
    double iOStallCount;
    ProcessorMetricsV1CacheMetrics coreCache;
    ProcessorMetricsV1CStateResidency cStateResidency;
};
struct ProcessorMetricsV1ProcessorMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    double bandwidthPercent;
    double averageFrequencyMHz;
    double throttlingCelsius;
    double temperatureCelsius;
    double consumedPowerWatt;
    double frequencyRatio;
    ProcessorMetricsV1CacheMetrics cache;
    int64_t localMemoryBandwidthBytes;
    int64_t remoteMemoryBandwidthBytes;
    double kernelPercent;
    double userPercent;
    ProcessorMetricsV1CoreMetrics coreMetrics;
    ProcessorMetricsV1Actions actions;
    int64_t operatingSpeedMHz;
    ProcessorMetricsV1CacheMetricsTotal cacheMetricsTotal;
};
#endif
