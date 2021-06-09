#ifndef PCIEDEVICECOLLECTION_V1
#define PCIEDEVICECOLLECTION_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"

struct PCIeDeviceCollection_v1_PCIeDeviceCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference__ members;
};
#endif
