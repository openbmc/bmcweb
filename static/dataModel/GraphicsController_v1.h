#ifndef GRAPHICSCONTROLLER_V1
#define GRAPHICSCONTROLLER_V1

#include "GraphicsController_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"

struct GraphicsControllerV1OemActions
{};
struct GraphicsControllerV1Actions
{
    GraphicsControllerV1OemActions oem;
};
struct GraphicsControllerV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ processors;
    NavigationReference_ pCIeDevice;
};
struct GraphicsControllerV1GraphicsController
{
    ResourceV1Resource oem;
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
    ResourceV1Resource status;
    ResourceV1Resource location;
    NavigationReference_ ports;
    GraphicsControllerV1Links links;
    GraphicsControllerV1Actions actions;
};
#endif
