#ifndef THERMALSUBSYSTEM_V1
#define THERMALSUBSYSTEM_V1

#include "NavigationReference.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "ThermalSubsystem_v1.h"

struct ThermalSubsystem_v1_Actions
{
    ThermalSubsystem_v1_OemActions oem;
};
struct ThermalSubsystem_v1_OemActions
{};
struct ThermalSubsystem_v1_ThermalSubsystem
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    NavigationReference_ fans;
    Redundancy_v1_Redundancy fanRedundancy;
    NavigationReference_ thermalMetrics;
    ThermalSubsystem_v1_Actions actions;
};
#endif
