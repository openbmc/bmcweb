#ifndef POWERSUBSYSTEM_V1
#define POWERSUBSYSTEM_V1

#include "NavigationReferenceRedfish.h"
#include "PowerSubsystem_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

struct PowerSubsystemV1OemActions
{};
struct PowerSubsystemV1Actions
{
    PowerSubsystemV1OemActions oem;
};
struct PowerSubsystemV1PowerAllocation
{
    double requestedWatts;
    double allocatedWatts;
};
struct PowerSubsystemV1PowerSubsystem
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    double capacityWatts;
    PowerSubsystemV1PowerAllocation allocation;
    NavigationReferenceRedfish powerSupplies;
    RedundancyV1Redundancy powerSupplyRedundancy;
    PowerSubsystemV1Actions actions;
};
#endif
