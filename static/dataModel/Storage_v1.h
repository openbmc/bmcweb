#ifndef STORAGE_V1
#define STORAGE_V1

#include "Assembly_v1.h"
#include "ConsistencyGroupCollection_v1.h"
#include "Drive_v1.h"
#include "EndpointGroupCollection_v1.h"
#include "FileSystemCollection_v1.h"
#include "NavigationReference__.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Protocol_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "StorageControllerCollection_v1.h"
#include "StoragePoolCollection_v1.h"
#include "Storage_v1.h"
#include "VolumeCollection_v1.h"
#include "Volume_v1.h"

struct Storage_v1_Actions
{
    Storage_v1_OemActions oem;
};
struct Storage_v1_CacheSummary
{
    int64_t totalCacheSizeMiB;
    int64_t persistentCacheSizeMiB;
    Resource_v1_Resource status;
};
struct Storage_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ enclosures;
    NavigationReference__ simpleStorage;
    NavigationReference__ storageServices;
};
struct Storage_v1_OemActions
{};
struct Storage_v1_Rates
{
    int64_t rebuildRatePercent;
    int64_t transformationRatePercent;
    int64_t consistencyCheckRatePercent;
};
struct Storage_v1_Storage
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Storage_v1_StorageController storageControllers;
    Drive_v1_Drive drives;
    VolumeCollection_v1_VolumeCollection volumes;
    Storage_v1_Links links;
    Storage_v1_Actions actions;
    Resource_v1_Resource status;
    Redundancy_v1_Redundancy redundancy;
    FileSystemCollection_v1_FileSystemCollection fileSystems;
    StoragePoolCollection_v1_StoragePoolCollection storagePools;
    EndpointGroupCollection_v1_EndpointGroupCollection endpointGroups;
    ConsistencyGroupCollection_v1_ConsistencyGroupCollection consistencyGroups;
    StorageControllerCollection_v1_StorageControllerCollection controllers;
    Resource_v1_Resource identifiers;
};
struct Storage_v1_StorageController
{
    Resource_v1_Resource oem;
    std::string memberId;
    Resource_v1_Resource status;
    double speedGbps;
    std::string firmwareVersion;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    Protocol_v1_Protocol supportedControllerProtocols;
    Protocol_v1_Protocol supportedDeviceProtocols;
    Resource_v1_Resource identifiers;
    Storage_v1_StorageControllerLinks links;
    Storage_v1_StorageControllerActions actions;
    std::string name;
    Resource_v1_Resource location;
    Assembly_v1_Assembly assembly;
    Storage_v1_CacheSummary cacheSummary;
    PCIeDevice_v1_PCIeDevice pCIeInterface;
    Volume_v1_Volume supportedRAIDTypes;
    PortCollection_v1_PortCollection ports;
    Storage_v1_Rates controllerRates;
};
struct Storage_v1_StorageControllerActions
{
    Storage_v1_StorageControllerOemActions oem;
};
struct Storage_v1_StorageControllerLinks
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ storageServices;
    NavigationReference__ pCIeFunctions;
};
struct Storage_v1_StorageControllerOemActions
{};
#endif
