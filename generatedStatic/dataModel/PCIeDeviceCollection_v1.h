#ifndef PCIEDEVICECOLLECTION_V1
#define PCIEDEVICECOLLECTION_V1

#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"

struct PCIeDeviceCollectionV1PCIeDeviceCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReferenceRedfish members;
};
#endif
