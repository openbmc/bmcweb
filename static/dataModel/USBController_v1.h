#ifndef USBCONTROLLER_V1
#define USBCONTROLLER_V1

#include "NavigationReference.h"
#include "Resource_v1.h"
#include "USBController_v1.h"

struct USBController_v1_Actions
{
    USBController_v1_OemActions oem;
};
struct USBController_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference_ processors;
    NavigationReference_ pCIeDevice;
};
struct USBController_v1_OemActions
{};
struct USBController_v1_USBController
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    Resource_v1_Resource status;
    NavigationReference_ ports;
    USBController_v1_Links links;
    USBController_v1_Actions actions;
};
#endif
