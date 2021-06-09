#ifndef STORAGEPOOL_V1
#define STORAGEPOOL_V1

#include "Capacity_v1.h"
#include "ClassOfServiceCollection_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "IOStatistics_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StoragePool_v1.h"
#include "Volume_v1.h"
#include "VolumeCollection_v1.h"

struct StoragePool_v1_Actions
{
    StoragePool_v1_OemActions oem;
};
struct StoragePool_v1_EndGrpLifetime
{
    int64_t percentUsed;
    int64_t enduranceEstimate;
    int64_t dataUnitsRead;
    int64_t dataUnitsWritten;
    int64_t mediaUnitsWritten;
    int64_t hostReadCommandCount;
    int64_t hostWriteCommandCount;
    int64_t mediaAndDataIntegrityErrorCount;
    int64_t errorInformationLogEntryCount;
};
struct StoragePool_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ defaultClassOfService;
    NavigationReference__ owningStorageResource;
};
struct StoragePool_v1_NVMeEnduranceGroupProperties
{
    StoragePool_v1_EndGrpLifetime endGrpLifetime;
    double predictedMediaLifeLeftPercent;
};
struct StoragePool_v1_NVMeSetProperties
{
    std::string setIdentifier;
    int64_t optimalWriteSizeBytes;
    std::string enduranceGroupIdentifier;
    int64_t random4kReadTypicalNanoSeconds;
    int64_t unallocatedNVMNamespaceCapacityBytes;
};
struct StoragePool_v1_OemActions
{
};
struct StoragePool_v1_StoragePool
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    int64_t blockSizeBytes;
    Capacity_v1_Capacity capacity;
    int64_t lowSpaceWarningThresholdPercents;
    VolumeCollection_v1_VolumeCollection allocatedVolumes;
    ClassOfServiceCollection_v1_ClassOfServiceCollection classesOfService;
    StoragePool_v1_Links links;
    Resource_v1_Resource status;
    int64_t remainingCapacityPercent;
    int64_t maxBlockSizeBytes;
    IOStatistics_v1_IOStatistics iOStatistics;
    int64_t recoverableCapacitySourceCount;
    NavigationReference__ defaultClassOfService;
    Volume_v1_Volume supportedRAIDTypes;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities supportedProvisioningPolicies;
    bool deduplicated;
    bool compressed;
    bool encrypted;
    StoragePool_v1_Actions actions;
    StoragePool_v1_NVMeSetProperties nVMeSetProperties;
    StoragePool_v1_NVMeEnduranceGroupProperties nVMeEnduranceGroupProperties;
};
#endif
