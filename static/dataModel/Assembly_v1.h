#ifndef ASSEMBLY_V1
#define ASSEMBLY_V1

#include <chrono>
#include "Assembly_v1.h"
#include "PhysicalContext_v1.h"
#include "Resource_v1.h"

struct Assembly_v1_Actions
{
    Assembly_v1_OemActions oem;
};
struct Assembly_v1_Assembly
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Assembly_v1_AssemblyData assemblies;
    Assembly_v1_Actions actions;
};
struct Assembly_v1_AssemblyData
{
    Resource_v1_Resource oem;
    std::string memberId;
    std::string name;
    std::string description;
    std::string model;
    std::string partNumber;
    std::string sparePartNumber;
    std::string SKU;
    std::string vendor;
    std::chrono::time_point productionDate;
    std::string producer;
    std::string version;
    std::string engineeringChangeLevel;
    std::string binaryDataURI;
    Assembly_v1_AssemblyDataActions actions;
    Resource_v1_Resource status;
    std::string serialNumber;
    PhysicalContext_v1_PhysicalContext physicalContext;
    bool locationIndicatorActive;
    Resource_v1_Resource location;
};
struct Assembly_v1_AssemblyDataActions
{
    Assembly_v1_AssemblyDataOemActions oem;
};
struct Assembly_v1_AssemblyDataOemActions
{
};
struct Assembly_v1_OemActions
{
};
#endif
