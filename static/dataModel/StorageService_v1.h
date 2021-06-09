#ifndef STORAGESERVICE_V1
#define STORAGESERVICE_V1

#include "ClassOfServiceCollection_v1.h"
#include "ConsistencyGroupCollection_v1.h"
#include "DataProtectionLoSCapabilities_v1.h"
#include "DataSecurityLoSCapabilities_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "DriveCollection_v1.h"
#include "EndpointCollection_v1.h"
#include "EndpointGroupCollection_v1.h"
#include "FileSystemCollection_v1.h"
#include "IOConnectivityLoSCapabilities_v1.h"
#include "IOPerformanceLoSCapabilities_v1.h"
#include "IOStatistics_v1.h"
#include "LineOfServiceCollection_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SpareResourceSet_v1.h"
#include "StorageCollection_v1.h"
#include "StoragePoolCollection_v1.h"
#include "StorageService_v1.h"
#include "VolumeCollection_v1.h"

struct StorageServiceV1OemActions
{};
struct StorageServiceV1Actions
{
    StorageServiceV1OemActions oem;
};
struct StorageServiceV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish hostingSystem;
    NavigationReferenceRedfish defaultClassOfService;
    NavigationReferenceRedfish dataProtectionLoSCapabilities;
    NavigationReferenceRedfish dataSecurityLoSCapabilities;
    NavigationReferenceRedfish dataStorageLoSCapabilities;
    NavigationReferenceRedfish iOConnectivityLoSCapabilities;
    NavigationReferenceRedfish iOPerformanceLoSCapabilities;
};
struct StorageServiceV1StorageService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    ResourceV1Resource status;
    StorageServiceV1Links links;
    EndpointGroupCollectionV1EndpointGroupCollection endpointGroups;
    EndpointGroupCollectionV1EndpointGroupCollection clientEndpointGroups;
    EndpointGroupCollectionV1EndpointGroupCollection serverEndpointGroups;
    VolumeCollectionV1VolumeCollection volumes;
    FileSystemCollectionV1FileSystemCollection fileSystems;
    StoragePoolCollectionV1StoragePoolCollection storagePools;
    DriveCollectionV1DriveCollection drives;
    EndpointCollectionV1EndpointCollection endpoints;
    StorageServiceV1Actions actions;
    RedundancyV1Redundancy redundancy;
    ClassOfServiceCollectionV1ClassOfServiceCollection classesOfService;
    StorageCollectionV1StorageCollection storageSubsystems;
    IOStatisticsV1IOStatistics iOStatistics;
    SpareResourceSetV1SpareResourceSet spareResourceSets;
    DataProtectionLoSCapabilitiesV1DataProtectionLoSCapabilities
        dataProtectionLoSCapabilities;
    DataSecurityLoSCapabilitiesV1DataSecurityLoSCapabilities
        dataSecurityLoSCapabilities;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities
        dataStorageLoSCapabilities;
    IOConnectivityLoSCapabilitiesV1IOConnectivityLoSCapabilities
        iOConnectivityLoSCapabilities;
    IOPerformanceLoSCapabilitiesV1IOPerformanceLoSCapabilities
        iOPerformanceLoSCapabilities;
    NavigationReferenceRedfish defaultClassOfService;
    ConsistencyGroupCollectionV1ConsistencyGroupCollection consistencyGroups;
    LineOfServiceCollectionV1LineOfServiceCollection linesOfService;
};
#endif
