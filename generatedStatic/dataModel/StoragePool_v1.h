#ifndef STORAGEPOOL_V1
#define STORAGEPOOL_V1

#include "Capacity_v1.h"
#include "ClassOfServiceCollection_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "IOStatistics_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"
#include "StoragePool_v1.h"
#include "VolumeCollection_v1.h"
#include "Volume_v1.h"

enum class StoragePoolV1NVMePoolType
{
    EnduranceGroup,
    NVMSet,
};
enum class StoragePoolV1PoolType
{
    Block,
    File,
    Object,
    Pool,
};
struct StoragePoolV1OemActions
{};
struct StoragePoolV1Actions
{
    StoragePoolV1OemActions oem;
};
struct StoragePoolV1EndGrpLifetime
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
struct StoragePoolV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish defaultClassOfService;
    NavigationReferenceRedfish owningStorageResource;
};
struct StoragePoolV1NVMeEnduranceGroupProperties
{
    StoragePoolV1EndGrpLifetime endGrpLifetime;
    double predictedMediaLifeLeftPercent;
};
struct StoragePoolV1NVMeProperties
{
    StoragePoolV1NVMePoolType nVMePoolType;
};
struct StoragePoolV1NVMeSetProperties
{
    std::string setIdentifier;
    int64_t optimalWriteSizeBytes;
    std::string enduranceGroupIdentifier;
    int64_t random4kReadTypicalNanoSeconds;
    int64_t unallocatedNVMNamespaceCapacityBytes;
};
struct StoragePoolV1StoragePool
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    int64_t blockSizeBytes;
    CapacityV1Capacity capacity;
    int64_t lowSpaceWarningThresholdPercents;
    VolumeCollectionV1VolumeCollection allocatedVolumes;
    ClassOfServiceCollectionV1ClassOfServiceCollection classesOfService;
    StoragePoolV1Links links;
    ResourceV1Resource status;
    int64_t remainingCapacityPercent;
    int64_t maxBlockSizeBytes;
    IOStatisticsV1IOStatistics iOStatistics;
    int64_t recoverableCapacitySourceCount;
    NavigationReferenceRedfish defaultClassOfService;
    VolumeV1Volume supportedRAIDTypes;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities
        supportedProvisioningPolicies;
    bool deduplicated;
    bool compressed;
    bool encrypted;
    StoragePoolV1Actions actions;
    StoragePoolV1NVMeSetProperties nVMeSetProperties;
    StoragePoolV1NVMeEnduranceGroupProperties nVMeEnduranceGroupProperties;
    bool defaultCompressionBehavior;
    bool defaultEncryptionBehavior;
    bool defaultDeduplicationBehavior;
    bool deduplicationEnabled;
    bool compressionEnabled;
    bool encryptionEnabled;
    StoragePoolV1PoolType poolType;
    StoragePoolV1NVMeProperties nVMeProperties;
};
#endif
