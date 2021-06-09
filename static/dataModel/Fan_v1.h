#ifndef FAN_V1
#define FAN_V1

#include "Assembly_v1.h"
#include "Fan_v1.h"
#include "NavigationReference.h"
#include "PhysicalContext_v1.h"
#include "Resource_v1.h"

struct Fan_v1_Actions
{
    Fan_v1_OemActions oem;
};
struct Fan_v1_Fan
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PhysicalContext_v1_PhysicalContext physicalContext;
    Resource_v1_Resource status;
    NavigationReference_ speedPercent;
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    Resource_v1_Resource location;
    bool locationIndicatorActive;
    bool hotPluggable;
    Assembly_v1_Assembly assembly;
    Fan_v1_Actions actions;
};
struct Fan_v1_OemActions
{
};
#endif
