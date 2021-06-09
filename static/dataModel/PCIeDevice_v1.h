#ifndef PCIEDEVICE_V1
#define PCIEDEVICE_V1

#include "NavigationReference_.h"
#include "PCIeDevice_v1.h"
#include "PCIeFunctionCollection_v1.h"
#include "Resource_v1.h"

enum class PCIeDeviceV1DeviceType
{
    SingleFunction,
    MultiFunction,
    Simulated,
};
enum class PCIeDeviceV1PCIeTypes
{
    Gen1,
    Gen2,
    Gen3,
    Gen4,
    Gen5,
};
struct PCIeDeviceV1OemActions
{};
struct PCIeDeviceV1Actions
{
    PCIeDeviceV1OemActions oem;
};
struct PCIeDeviceV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ chassis;
    NavigationReference_ pCIeFunctions;
};
struct PCIeDeviceV1PCIeInterface
{
    ResourceV1Resource oem;
    PCIeDeviceV1PCIeTypes maxPCIeType;
    PCIeDeviceV1PCIeTypes pCIeType;
    int64_t maxLanes;
    int64_t lanesInUse;
};
struct PCIeDeviceV1PCIeDevice
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
    std::string assetTag;
    PCIeDeviceV1DeviceType deviceType;
    std::string firmwareVersion;
    ResourceV1Resource status;
    PCIeDeviceV1Links links;
    PCIeDeviceV1Actions actions;
    NavigationReference_ assembly;
    PCIeDeviceV1PCIeInterface pCIeInterface;
    PCIeFunctionCollectionV1PCIeFunctionCollection pCIeFunctions;
    std::string UUID;
    std::string sparePartNumber;
    bool readyToRemove;
    NavigationReference_ environmentMetrics;
};
#endif
