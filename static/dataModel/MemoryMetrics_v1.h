#ifndef MEMORYMETRICS_V1
#define MEMORYMETRICS_V1

#include "MemoryMetrics_v1.h"
#include "Resource_v1.h"

struct MemoryMetricsV1OemActions
{};
struct MemoryMetricsV1Actions
{
    MemoryMetricsV1OemActions oem;
};
struct MemoryMetricsV1AlarmTrips
{
    bool temperature;
    bool spareBlock;
    bool uncorrectableECCError;
    bool correctableECCError;
    bool addressParityError;
};
struct MemoryMetricsV1CurrentPeriod
{
    int64_t blocksRead;
    int64_t blocksWritten;
    int64_t correctableECCErrorCount;
    int64_t uncorrectableECCErrorCount;
};
struct MemoryMetricsV1HealthData
{
    double remainingSpareBlockPercentage;
    bool lastShutdownSuccess;
    bool dataLossDetected;
    bool performanceDegraded;
    MemoryMetricsV1AlarmTrips alarmTrips;
    double predictedMediaLifeLeftPercent;
};
struct MemoryMetricsV1LifeTime
{
    int64_t blocksRead;
    int64_t blocksWritten;
    int64_t correctableECCErrorCount;
    int64_t uncorrectableECCErrorCount;
};
struct MemoryMetricsV1MemoryMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t blockSizeBytes;
    MemoryMetricsV1CurrentPeriod currentPeriod;
    MemoryMetricsV1LifeTime lifeTime;
    MemoryMetricsV1HealthData healthData;
    MemoryMetricsV1Actions actions;
    double bandwidthPercent;
    int64_t operatingSpeedMHz;
};
#endif
