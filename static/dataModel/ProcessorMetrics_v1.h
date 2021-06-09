#ifndef PROCESSORMETRICS_V1
#define PROCESSORMETRICS_V1

#include "ProcessorMetrics_v1.h"
#include "Resource_v1.h"

struct ProcessorMetrics_v1_Actions
{
    ProcessorMetrics_v1_OemActions oem;
};
struct ProcessorMetrics_v1_CacheMetrics
{
    std::string level;
    double cacheMiss;
    double hitRatio;
    double cacheMissesPerInstruction;
    int64_t occupancyBytes;
    double occupancyPercent;
};
struct ProcessorMetrics_v1_CoreMetrics
{
    std::string coreId;
    double instructionsPerCycle;
    double unhaltedCycles;
    double memoryStallCount;
    double iOStallCount;
    ProcessorMetrics_v1_CacheMetrics coreCache;
    ProcessorMetrics_v1_CStateResidency cStateResidency;
};
struct ProcessorMetrics_v1_CStateResidency
{
    std::string level;
    double residencyPercent;
};
struct ProcessorMetrics_v1_OemActions
{};
struct ProcessorMetrics_v1_ProcessorMetrics
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    double bandwidthPercent;
    double averageFrequencyMHz;
    double throttlingCelsius;
    double temperatureCelsius;
    double consumedPowerWatt;
    double frequencyRatio;
    ProcessorMetrics_v1_CacheMetrics cache;
    int64_t localMemoryBandwidthBytes;
    int64_t remoteMemoryBandwidthBytes;
    double kernelPercent;
    double userPercent;
    ProcessorMetrics_v1_CoreMetrics coreMetrics;
    ProcessorMetrics_v1_Actions actions;
    int64_t operatingSpeedMHz;
};
#endif
