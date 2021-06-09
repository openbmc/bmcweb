#ifndef NETWORKADAPTER_V1
#define NETWORKADAPTER_V1

#include "Assembly_v1.h"
#include "NavigationReference__.h"
#include "NetworkAdapter_v1.h"
#include "NetworkDeviceFunctionCollection_v1.h"
#include "NetworkPortCollection_v1.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"

struct NetworkAdapter_v1_Actions
{
    NetworkAdapter_v1_OemActions oem;
};
struct NetworkAdapter_v1_ControllerCapabilities
{
    int64_t networkPortCount;
    int64_t networkDeviceFunctionCount;
    NetworkAdapter_v1_DataCenterBridging dataCenterBridging;
    NetworkAdapter_v1_VirtualizationOffload virtualizationOffload;
    NetworkAdapter_v1_NPIV NPIV;
    NetworkAdapter_v1_NicPartitioning NPAR;
};
struct NetworkAdapter_v1_ControllerLinks
{
    Resource_v1_Resource oem;
    NavigationReference__ pCIeDevices;
    NavigationReference__ networkPorts;
    NavigationReference__ networkDeviceFunctions;
    NavigationReference__ ports;
};
struct NetworkAdapter_v1_Controllers
{
    std::string firmwarePackageVersion;
    NetworkAdapter_v1_ControllerLinks links;
    NetworkAdapter_v1_ControllerCapabilities controllerCapabilities;
    Resource_v1_Resource location;
    PCIeDevice_v1_PCIeDevice pCIeInterface;
    Resource_v1_Resource identifiers;
};
struct NetworkAdapter_v1_DataCenterBridging
{
    bool capable;
};
struct NetworkAdapter_v1_NetworkAdapter
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    NetworkPortCollection_v1_NetworkPortCollection networkPorts;
    NetworkDeviceFunctionCollection_v1_NetworkDeviceFunctionCollection
        networkDeviceFunctions;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    NetworkAdapter_v1_Controllers controllers;
    NetworkAdapter_v1_Actions actions;
    Assembly_v1_Assembly assembly;
    Resource_v1_Resource location;
    Resource_v1_Resource identifiers;
    PortCollection_v1_PortCollection ports;
};
struct NetworkAdapter_v1_NicPartitioning
{
    bool nparCapable;
    bool nparEnabled;
};
struct NetworkAdapter_v1_NPIV
{
    int64_t maxDeviceLogins;
    int64_t maxPortLogins;
};
struct NetworkAdapter_v1_OemActions
{};
struct NetworkAdapter_v1_SRIOV
{
    bool sRIOVVEPACapable;
};
struct NetworkAdapter_v1_VirtualFunction
{
    int64_t deviceMaxCount;
    int64_t networkPortMaxCount;
    int64_t minAssignmentGroupSize;
};
struct NetworkAdapter_v1_VirtualizationOffload
{
    NetworkAdapter_v1_VirtualFunction virtualFunction;
    NetworkAdapter_v1_SRIOV SRIOV;
};
#endif
