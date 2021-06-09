#ifndef ASSEMBLY_V1
#define ASSEMBLY_V1

#include "Assembly_v1.h"
#include "PhysicalContext_v1.h"
#include "Resource_v1.h"

#include <chrono>

struct AssemblyV1OemActions
{};
struct AssemblyV1Actions
{
    AssemblyV1OemActions oem;
};
struct AssemblyV1AssemblyDataOemActions
{};
struct AssemblyV1AssemblyDataActions
{
    AssemblyV1AssemblyDataOemActions oem;
};
struct AssemblyV1AssemblyData
{
    ResourceV1Resource oem;
    std::string memberId;
    std::string name;
    std::string description;
    std::string model;
    std::string partNumber;
    std::string sparePartNumber;
    std::string SKU;
    std::string vendor;
    std::chrono::time_point<std::chrono::system_clock> productionDate;
    std::string producer;
    std::string version;
    std::string engineeringChangeLevel;
    std::string binaryDataURI;
    AssemblyV1AssemblyDataActions actions;
    ResourceV1Resource status;
    std::string serialNumber;
    PhysicalContextV1PhysicalContext physicalContext;
    bool locationIndicatorActive;
    ResourceV1Resource location;
};
struct AssemblyV1Assembly
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    AssemblyV1AssemblyData assemblies;
    AssemblyV1Actions actions;
};
#endif
