#ifndef POWERSUBSYSTEM_V1
#define POWERSUBSYSTEM_V1

#include "NavigationReference.h"
#include "PowerSubsystem_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

struct PowerSubsystem_v1_Actions
{
    PowerSubsystem_v1_OemActions oem;
};
struct PowerSubsystem_v1_OemActions
{};
struct PowerSubsystem_v1_PowerAllocation
{
    double requestedWatts;
    double allocatedWatts;
};
struct PowerSubsystem_v1_PowerSubsystem
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    double capacityWatts;
    PowerSubsystem_v1_PowerAllocation allocation;
    NavigationReference_ powerSupplies;
    Redundancy_v1_Redundancy powerSupplyRedundancy;
    PowerSubsystem_v1_Actions actions;
};
#endif
