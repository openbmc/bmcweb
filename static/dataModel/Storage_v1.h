#ifndef STORAGE_V1
#define STORAGE_V1

#include "Assembly_v1.h"
#include "CertificateCollection_v1.h"
#include "ConsistencyGroupCollection_v1.h"
#include "Drive_v1.h"
#include "EndpointGroupCollection_v1.h"
#include "FileSystemCollection_v1.h"
#include "NavigationReference_.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Protocol_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"
#include "StorageControllerCollection_v1.h"
#include "StoragePoolCollection_v1.h"
#include "Storage_v1.h"
#include "VolumeCollection_v1.h"
#include "Volume_v1.h"

struct StorageV1OemActions
{};
struct StorageV1Actions
{
    StorageV1OemActions oem;
};
struct StorageV1CacheSummary
{
    int64_t totalCacheSizeMiB;
    int64_t persistentCacheSizeMiB;
    ResourceV1Resource status;
};
struct StorageV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ enclosures;
    NavigationReference_ simpleStorage;
    NavigationReference_ storageServices;
};
struct StorageV1Rates
{
    int64_t rebuildRatePercent;
    int64_t transformationRatePercent;
    int64_t consistencyCheckRatePercent;
};
struct StorageV1StorageControllerLinks
{
    ResourceV1Resource oem;
    NavigationReference_ endpoints;
    NavigationReference_ storageServices;
    NavigationReference_ pCIeFunctions;
};
struct StorageV1StorageControllerOemActions
{};
struct StorageV1StorageControllerActions
{
    StorageV1StorageControllerOemActions oem;
};
struct StorageV1StorageController
{
    ResourceV1Resource oem;
    std::string memberId;
    ResourceV1Resource status;
    double speedGbps;
    std::string firmwareVersion;
    std::string manufacturer;
    std::string model;
    std::string SKU;
    std::string serialNumber;
    std::string partNumber;
    std::string assetTag;
    ProtocolV1Protocol supportedControllerProtocols;
    ProtocolV1Protocol supportedDeviceProtocols;
    ResourceV1Resource identifiers;
    StorageV1StorageControllerLinks links;
    StorageV1StorageControllerActions actions;
    std::string name;
    ResourceV1Resource location;
    AssemblyV1Assembly assembly;
    StorageV1CacheSummary cacheSummary;
    PCIeDeviceV1PCIeDevice pCIeInterface;
    VolumeV1Volume supportedRAIDTypes;
    PortCollectionV1PortCollection ports;
    StorageV1Rates controllerRates;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
};
struct StorageV1Storage
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    StorageV1StorageController storageControllers;
    DriveV1Drive drives;
    VolumeCollectionV1VolumeCollection volumes;
    StorageV1Links links;
    StorageV1Actions actions;
    ResourceV1Resource status;
    RedundancyV1Redundancy redundancy;
    FileSystemCollectionV1FileSystemCollection fileSystems;
    StoragePoolCollectionV1StoragePoolCollection storagePools;
    EndpointGroupCollectionV1EndpointGroupCollection endpointGroups;
    ConsistencyGroupCollectionV1ConsistencyGroupCollection consistencyGroups;
    StorageControllerCollectionV1StorageControllerCollection controllers;
    ResourceV1Resource identifiers;
};
#endif
