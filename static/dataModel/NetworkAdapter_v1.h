#ifndef NETWORKADAPTER_V1
#define NETWORKADAPTER_V1

#include "Assembly_v1.h"
#include "CertificateCollection_v1.h"
#include "NavigationReferenceRedfish.h"
#include "NetworkAdapter_v1.h"
#include "NetworkDeviceFunctionCollection_v1.h"
#include "NetworkPortCollection_v1.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"

struct NetworkAdapterV1OemActions
{};
struct NetworkAdapterV1Actions
{
    NetworkAdapterV1OemActions oem;
};
struct NetworkAdapterV1DataCenterBridging
{
    bool capable;
};
struct NetworkAdapterV1VirtualFunction
{
    int64_t deviceMaxCount;
    int64_t networkPortMaxCount;
    int64_t minAssignmentGroupSize;
};
struct NetworkAdapterV1SRIOV
{
    bool sRIOVVEPACapable;
};
struct NetworkAdapterV1VirtualizationOffload
{
    NetworkAdapterV1VirtualFunction virtualFunction;
    NetworkAdapterV1SRIOV SRIOV;
};
struct NetworkAdapterV1NPIV
{
    int64_t maxDeviceLogins;
    int64_t maxPortLogins;
};
struct NetworkAdapterV1NicPartitioning
{
    bool nparCapable;
    bool nparEnabled;
};
struct NetworkAdapterV1ControllerCapabilities
{
    int64_t networkPortCount;
    int64_t networkDeviceFunctionCount;
    NetworkAdapterV1DataCenterBridging dataCenterBridging;
    NetworkAdapterV1VirtualizationOffload virtualizationOffload;
    NetworkAdapterV1NPIV NPIV;
    NetworkAdapterV1NicPartitioning NPAR;
};
struct NetworkAdapterV1ControllerLinks
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish pCIeDevices;
    NavigationReferenceRedfish networkPorts;
    NavigationReferenceRedfish networkDeviceFunctions;
    NavigationReferenceRedfish ports;
};
struct NetworkAdapterV1Controllers
{
    std::string firmwarePackageVersion;
    NetworkAdapterV1ControllerLinks links;
    NetworkAdapterV1ControllerCapabilities controllerCapabilities;
    ResourceV1Resource location;
    PCIeDeviceV1PCIeDevice pCIeInterface;
    ResourceV1Resource identifiers;
};
struct NetworkAdapterV1NetworkAdapter
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NetworkPortCollectionV1NetworkPortCollection networkPorts;
    NetworkDeviceFunctionCollectionV1NetworkDeviceFunctionCollection
        networkDeviceFunctions;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    NetworkAdapterV1Controllers controllers;
    NetworkAdapterV1Actions actions;
    AssemblyV1Assembly assembly;
    ResourceV1Resource location;
    ResourceV1Resource identifiers;
    PortCollectionV1PortCollection ports;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
    NavigationReferenceRedfish metrics;
    NavigationReferenceRedfish environmentMetrics;
    bool lLDPEnabled;
};
#endif
