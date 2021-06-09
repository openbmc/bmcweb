#ifndef STORAGECONTROLLER_V1
#define STORAGECONTROLLER_V1

#include "Assembly_v1.h"
#include "NavigationReference__.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"
#include "StorageController_v1.h"
#include "Volume_v1.h"

enum class StorageController_v1_ANAAccessState {
    Optimized,
    NonOptimized,
    Inacessible,
    PersistentLoss,
};
enum class StorageController_v1_NVMeControllerType {
    Admin,
    Discovery,
    IO,
};
struct StorageController_v1_Actions
{
    StorageController_v1_OemActions oem;
};
struct StorageController_v1_ANACharacteristics
{
    StorageController_v1_ANAAccessState accessState;
    NavigationReference__ volume;
};
struct StorageController_v1_CacheSummary
{
    int64_t totalCacheSizeMiB;
    int64_t persistentCacheSizeMiB;
    Resource_v1_Resource status;
};
struct StorageController_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ endpoints;
    NavigationReference__ pCIeFunctions;
    NavigationReference__ attachedVolumes;
};
struct StorageController_v1_NVMeControllerAttributes
{
    bool reportsUUIDList;
    bool supportsSQAssociations;
    bool reportsNamespaceGranularity;
    bool supportsTrafficBasedKeepAlive;
    bool supportsPredictableLatencyMode;
    bool supportsEnduranceGroups;
    bool supportsReadRecoveryLevels;
    bool supportsNVMSets;
    bool supportsExceedingPowerOfNonOperationalState;
    bool supports128BitHostId;
};
struct StorageController_v1_NVMeControllerProperties
{
    StorageController_v1_NVMeControllerType controllerType;
    std::string nVMeVersion;
    StorageController_v1_NVMeControllerAttributes nVMeControllerAttributes;
    int64_t maxQueueSize;
    StorageController_v1_ANACharacteristics aNACharacteristics;
    StorageController_v1_NVMeSMARTCriticalWarnings nVMeSMARTCriticalWarnings;
};
struct StorageController_v1_NVMeSMARTCriticalWarnings
{
    bool pMRUnreliable;
    bool powerBackupFailed;
    bool mediaInReadOnly;
    bool overallSubsystemDegraded;
    bool spareCapacityWornOut;
};
struct StorageController_v1_OemActions
{
};
struct StorageController_v1_Rates
{
    int64_t rebuildRatePercent;
    int64_t transformationRatePercent;
    int64_t consistencyCheckRatePercent;
};
struct StorageController_v1_StorageController
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
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
    Resource_v1_Resource location;
    Assembly_v1_Assembly assembly;
    StorageController_v1_CacheSummary cacheSummary;
    PCIeDevice_v1_PCIeDevice pCIeInterface;
    Volume_v1_Volume supportedRAIDTypes;
    PortCollection_v1_PortCollection ports;
    StorageController_v1_Rates controllerRates;
    StorageController_v1_NVMeControllerProperties nVMeControllerProperties;
    StorageController_v1_Links links;
    StorageController_v1_Actions actions;
};
#endif
