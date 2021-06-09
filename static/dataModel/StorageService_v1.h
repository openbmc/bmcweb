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
#include "NavigationReference__.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "SpareResourceSet_v1.h"
#include "StorageCollection_v1.h"
#include "StoragePoolCollection_v1.h"
#include "StorageService_v1.h"
#include "VolumeCollection_v1.h"

struct StorageService_v1_Actions
{
    StorageService_v1_OemActions oem;
};
struct StorageService_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ hostingSystem;
    NavigationReference__ defaultClassOfService;
    NavigationReference__ dataProtectionLoSCapabilities;
    NavigationReference__ dataSecurityLoSCapabilities;
    NavigationReference__ dataStorageLoSCapabilities;
    NavigationReference__ iOConnectivityLoSCapabilities;
    NavigationReference__ iOPerformanceLoSCapabilities;
};
struct StorageService_v1_OemActions
{
};
struct StorageService_v1_StorageService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    Resource_v1_Resource status;
    StorageService_v1_Links links;
    EndpointGroupCollection_v1_EndpointGroupCollection endpointGroups;
    EndpointGroupCollection_v1_EndpointGroupCollection clientEndpointGroups;
    EndpointGroupCollection_v1_EndpointGroupCollection serverEndpointGroups;
    VolumeCollection_v1_VolumeCollection volumes;
    FileSystemCollection_v1_FileSystemCollection fileSystems;
    StoragePoolCollection_v1_StoragePoolCollection storagePools;
    DriveCollection_v1_DriveCollection drives;
    EndpointCollection_v1_EndpointCollection endpoints;
    StorageService_v1_Actions actions;
    Redundancy_v1_Redundancy redundancy;
    ClassOfServiceCollection_v1_ClassOfServiceCollection classesOfService;
    StorageCollection_v1_StorageCollection storageSubsystems;
    IOStatistics_v1_IOStatistics iOStatistics;
    SpareResourceSet_v1_SpareResourceSet spareResourceSets;
    DataProtectionLoSCapabilities_v1_DataProtectionLoSCapabilities dataProtectionLoSCapabilities;
    DataSecurityLoSCapabilities_v1_DataSecurityLoSCapabilities dataSecurityLoSCapabilities;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities dataStorageLoSCapabilities;
    IOConnectivityLoSCapabilities_v1_IOConnectivityLoSCapabilities iOConnectivityLoSCapabilities;
    IOPerformanceLoSCapabilities_v1_IOPerformanceLoSCapabilities iOPerformanceLoSCapabilities;
    NavigationReference__ defaultClassOfService;
    ConsistencyGroupCollection_v1_ConsistencyGroupCollection consistencyGroups;
    LineOfServiceCollection_v1_LineOfServiceCollection linesOfService;
};
#endif
