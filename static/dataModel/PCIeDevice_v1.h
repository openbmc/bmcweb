#ifndef PCIEDEVICE_V1
#define PCIEDEVICE_V1

#include "NavigationReference__.h"
#include "PCIeDevice_v1.h"
#include "PCIeFunctionCollection_v1.h"
#include "Resource_v1.h"

enum class PCIeDevice_v1_DeviceType
{
    SingleFunction,
    MultiFunction,
    Simulated,
};
enum class PCIeDevice_v1_PCIeTypes
{
    Gen1,
    Gen2,
    Gen3,
    Gen4,
    Gen5,
};
struct PCIeDevice_v1_Actions
{
    PCIeDevice_v1_OemActions oem;
};
struct PCIeDevice_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ chassis;
    NavigationReference__ pCIeFunctions;
};
struct PCIeDevice_v1_OemActions
{};
struct PCIeDevice_v1_PCIeDevice
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
    std::string assetTag;
    PCIeDevice_v1_DeviceType deviceType;
    std::string firmwareVersion;
    Resource_v1_Resource status;
    PCIeDevice_v1_Links links;
    PCIeDevice_v1_Actions actions;
    NavigationReference__ assembly;
    PCIeDevice_v1_PCIeInterface pCIeInterface;
    PCIeFunctionCollection_v1_PCIeFunctionCollection pCIeFunctions;
    string UUID;
};
struct PCIeDevice_v1_PCIeInterface
{
    Resource_v1_Resource oem;
    PCIeDevice_v1_PCIeTypes maxPCIeType;
    PCIeDevice_v1_PCIeTypes pCIeType;
    int64_t maxLanes;
    int64_t lanesInUse;
};
#endif
