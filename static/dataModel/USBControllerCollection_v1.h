#ifndef USBCONTROLLERCOLLECTION_V1
#define USBCONTROLLERCOLLECTION_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"

struct USBControllerCollectionV1USBControllerCollection
{
    std::string description;
    std::string name;
    ResourceV1Resource oem;
    NavigationReference_ members;
};
#endif
