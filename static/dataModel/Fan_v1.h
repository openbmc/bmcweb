#ifndef FAN_V1
#define FAN_V1

#include "Assembly_v1.h"
#include "Fan_v1.h"
#include "NavigationReferenceRedfish.h"
#include "PhysicalContext_v1.h"
#include "Resource_v1.h"

struct FanV1OemActions
{};
struct FanV1Actions
{
    FanV1OemActions oem;
};
struct FanV1Fan
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PhysicalContextV1PhysicalContext physicalContext;
    ResourceV1Resource status;
    NavigationReferenceRedfish speedPercent;
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    ResourceV1Resource location;
    bool locationIndicatorActive;
    bool hotPluggable;
    AssemblyV1Assembly assembly;
    FanV1Actions actions;
    NavigationReferenceRedfish powerWatts;
};
#endif
