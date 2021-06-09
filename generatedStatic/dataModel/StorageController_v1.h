#ifndef STORAGECONTROLLER_V1
#define STORAGECONTROLLER_V1

#include "Assembly_v1.h"
#include "CertificateCollection_v1.h"
#include "NavigationReferenceRedfish.h"
#include "PCIeDevice_v1.h"
#include "PortCollection_v1.h"
#include "Protocol_v1.h"
#include "Resource_v1.h"
#include "SoftwareInventory_v1.h"
#include "StorageController_v1.h"
#include "Volume_v1.h"

enum class StorageControllerV1ANAAccessState
{
    Optimized,
    NonOptimized,
    Inaccessible,
    PersistentLoss,
};
enum class StorageControllerV1NVMeControllerType
{
    Admin,
    Discovery,
    IO,
};
struct StorageControllerV1OemActions
{};
struct StorageControllerV1Actions
{
    StorageControllerV1OemActions oem;
};
struct StorageControllerV1ANACharacteristics
{
    StorageControllerV1ANAAccessState accessState;
    NavigationReferenceRedfish volume;
};
struct StorageControllerV1CacheSummary
{
    int64_t totalCacheSizeMiB;
    int64_t persistentCacheSizeMiB;
    ResourceV1Resource status;
};
struct StorageControllerV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish endpoints;
    NavigationReferenceRedfish pCIeFunctions;
    NavigationReferenceRedfish attachedVolumes;
};
struct StorageControllerV1NVMeControllerAttributes
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
    bool supportsReservations;
};
struct StorageControllerV1NVMeSMARTCriticalWarnings
{
    bool pMRUnreliable;
    bool powerBackupFailed;
    bool mediaInReadOnly;
    bool overallSubsystemDegraded;
    bool spareCapacityWornOut;
};
struct StorageControllerV1NVMeControllerProperties
{
    StorageControllerV1NVMeControllerType controllerType;
    std::string nVMeVersion;
    StorageControllerV1NVMeControllerAttributes nVMeControllerAttributes;
    int64_t maxQueueSize;
    StorageControllerV1ANACharacteristics aNACharacteristics;
    StorageControllerV1NVMeSMARTCriticalWarnings nVMeSMARTCriticalWarnings;
};
struct StorageControllerV1Rates
{
    int64_t rebuildRatePercent;
    int64_t transformationRatePercent;
    int64_t consistencyCheckRatePercent;
};
struct StorageControllerV1StorageController
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
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
    ResourceV1Resource location;
    AssemblyV1Assembly assembly;
    StorageControllerV1CacheSummary cacheSummary;
    PCIeDeviceV1PCIeDevice pCIeInterface;
    VolumeV1Volume supportedRAIDTypes;
    PortCollectionV1PortCollection ports;
    StorageControllerV1Rates controllerRates;
    StorageControllerV1NVMeControllerProperties nVMeControllerProperties;
    StorageControllerV1Links links;
    StorageControllerV1Actions actions;
    CertificateCollectionV1CertificateCollection certificates;
    SoftwareInventoryV1SoftwareInventory measurements;
    NavigationReferenceRedfish environmentMetrics;
};
#endif
