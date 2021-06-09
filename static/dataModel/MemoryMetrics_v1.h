#ifndef MEMORYMETRICS_V1
#define MEMORYMETRICS_V1

#include "MemoryMetrics_v1.h"
#include "Resource_v1.h"

struct MemoryMetrics_v1_Actions
{
    MemoryMetrics_v1_OemActions oem;
};
struct MemoryMetrics_v1_AlarmTrips
{
    bool temperature;
    bool spareBlock;
    bool uncorrectableECCError;
    bool correctableECCError;
    bool addressParityError;
};
struct MemoryMetrics_v1_CurrentPeriod
{
    int64_t blocksRead;
    int64_t blocksWritten;
    int64_t correctableECCErrorCount;
    int64_t uncorrectableECCErrorCount;
};
struct MemoryMetrics_v1_HealthData
{
    double remainingSpareBlockPercentage;
    bool lastShutdownSuccess;
    bool dataLossDetected;
    bool performanceDegraded;
    MemoryMetrics_v1_AlarmTrips alarmTrips;
    double predictedMediaLifeLeftPercent;
};
struct MemoryMetrics_v1_LifeTime
{
    int64_t blocksRead;
    int64_t blocksWritten;
    int64_t correctableECCErrorCount;
    int64_t uncorrectableECCErrorCount;
};
struct MemoryMetrics_v1_MemoryMetrics
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t blockSizeBytes;
    MemoryMetrics_v1_CurrentPeriod currentPeriod;
    MemoryMetrics_v1_LifeTime lifeTime;
    MemoryMetrics_v1_HealthData healthData;
    MemoryMetrics_v1_Actions actions;
    double bandwidthPercent;
    int64_t operatingSpeedMHz;
};
struct MemoryMetrics_v1_OemActions
{
};
#endif
