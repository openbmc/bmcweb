#ifndef USBCONTROLLERCOLLECTION_V1
#define USBCONTROLLERCOLLECTION_V1

#include "NavigationReference.h"
#include "Resource_v1.h"

struct USBControllerCollection_v1_USBControllerCollection
{
    std::string description;
    std::string name;
    Resource_v1_Resource oem;
    NavigationReference_ members;
};
#endif
