#ifndef THERMALSUBSYSTEM_V1
#define THERMALSUBSYSTEM_V1

#include "NavigationReference_.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "ThermalSubsystem_v1.h"

struct ThermalSubsystemV1OemActions
{};
struct ThermalSubsystemV1Actions
{
    ThermalSubsystemV1OemActions oem;
};
struct ThermalSubsystemV1ThermalSubsystem
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NavigationReference_ fans;
    RedundancyV1Redundancy fanRedundancy;
    NavigationReference_ thermalMetrics;
    ThermalSubsystemV1Actions actions;
};
#endif
