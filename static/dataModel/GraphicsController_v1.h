#ifndef GRAPHICSCONTROLLER_V1
#define GRAPHICSCONTROLLER_V1

#include "GraphicsController_v1.h"
#include "NavigationReference.h"
#include "Resource_v1.h"

struct GraphicsController_v1_Actions
{
    GraphicsController_v1_OemActions oem;
};
struct GraphicsController_v1_GraphicsController
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string assetTag;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    std::string biosVersion;
    std::string driverVersion;
    Resource_v1_Resource status;
    Resource_v1_Resource location;
    NavigationReference_ ports;
    GraphicsController_v1_Links links;
    GraphicsController_v1_Actions actions;
};
struct GraphicsController_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference_ processors;
    NavigationReference_ pCIeDevice;
};
struct GraphicsController_v1_OemActions
{};
#endif
