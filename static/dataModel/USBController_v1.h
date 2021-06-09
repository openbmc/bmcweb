#ifndef USBCONTROLLER_V1
#define USBCONTROLLER_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "USBController_v1.h"

struct USBControllerV1OemActions
{};
struct USBControllerV1Actions
{
    USBControllerV1OemActions oem;
};
struct USBControllerV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ processors;
    NavigationReference_ pCIeDevice;
};
struct USBControllerV1USBController
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    ResourceV1Resource status;
    NavigationReference_ ports;
    USBControllerV1Links links;
    USBControllerV1Actions actions;
};
#endif
