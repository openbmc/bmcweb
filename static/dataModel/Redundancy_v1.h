#ifndef REDUNDANCY_V1
#define REDUNDANCY_V1

#include "NavigationReference_.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

enum class RedundancyV1RedundancyType
{
    Failover,
    NPlusM,
    Sharing,
    Sparing,
    NotRedundant,
};
struct RedundancyV1OemActions
{};
struct RedundancyV1Actions
{
    RedundancyV1OemActions oem;
};
struct RedundancyV1Redundancy
{
    ResourceV1Resource oem;
    std::string memberId;
    std::string name;
    std::string mode;
    int64_t maxNumSupported;
    int64_t minNumNeeded;
    ResourceV1Resource status;
    NavigationReference_ redundancySet;
    bool redundancyEnabled;
    RedundancyV1Actions actions;
};
struct RedundancyV1RedundantGroup
{
    RedundancyV1RedundancyType redundancyType;
    int64_t maxSupportedInGroup;
    int64_t minNeededInGroup;
    ResourceV1Resource status;
    NavigationReference_ redundancyGroup;
};
#endif
