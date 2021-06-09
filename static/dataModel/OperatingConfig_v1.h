#ifndef OPERATINGCONFIG_V1
#define OPERATINGCONFIG_V1

#include "OperatingConfig_v1.h"
#include "Resource_v1.h"

struct OperatingConfig_v1_Actions
{
    OperatingConfig_v1_OemActions oem;
};
struct OperatingConfig_v1_BaseSpeedPrioritySettings
{
    int64_t coreCount;
    int64_t coreIDs;
    int64_t baseSpeedMHz;
};
struct OperatingConfig_v1_OemActions
{
};
struct OperatingConfig_v1_OperatingConfig
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t totalAvailableCoreCount;
    int64_t tDPWatts;
    int64_t baseSpeedMHz;
    int64_t maxSpeedMHz;
    int64_t maxJunctionTemperatureCelsius;
    OperatingConfig_v1_BaseSpeedPrioritySettings baseSpeedPrioritySettings;
    OperatingConfig_v1_TurboProfileDatapoint turboProfile;
    OperatingConfig_v1_Actions actions;
};
struct OperatingConfig_v1_TurboProfileDatapoint
{
    int64_t activeCoreCount;
    int64_t maxSpeedMHz;
};
#endif
