#ifndef OPERATINGCONFIG_V1
#define OPERATINGCONFIG_V1

#include "OperatingConfig_v1.h"
#include "Resource_v1.h"

struct OperatingConfigV1OemActions
{};
struct OperatingConfigV1Actions
{
    OperatingConfigV1OemActions oem;
};
struct OperatingConfigV1BaseSpeedPrioritySettings
{
    int64_t coreCount;
    int64_t coreIDs;
    int64_t baseSpeedMHz;
};
struct OperatingConfigV1TurboProfileDatapoint
{
    int64_t activeCoreCount;
    int64_t maxSpeedMHz;
};
struct OperatingConfigV1OperatingConfig
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t totalAvailableCoreCount;
    int64_t tDPWatts;
    int64_t baseSpeedMHz;
    int64_t maxSpeedMHz;
    int64_t maxJunctionTemperatureCelsius;
    OperatingConfigV1BaseSpeedPrioritySettings baseSpeedPrioritySettings;
    OperatingConfigV1TurboProfileDatapoint turboProfile;
    OperatingConfigV1Actions actions;
};
#endif
