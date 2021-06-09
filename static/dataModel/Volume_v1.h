#ifndef VOLUME_V1
#define VOLUME_V1

#include "Capacity_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "IOStatistics_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"
#include "Volume_v1.h"

enum class VolumeV1EncryptionTypes
{
    NativeDriveEncryption,
    ControllerAssisted,
    SoftwareAssisted,
};
enum class VolumeV1InitializeMethod
{
    Skip,
    Background,
    Foreground,
};
enum class VolumeV1InitializeType
{
    Fast,
    Slow,
};
enum class VolumeV1RAIDType
{
    RAID0,
    RAID1,
    RAID3,
    RAID4,
    RAID5,
    RAID6,
    RAID10,
    RAID01,
    RAID6TP,
    RAID1E,
    RAID50,
    RAID60,
    RAID00,
    RAID10E,
    RAID1Triple,
    RAID10Triple,
    None,
};
enum class VolumeV1ReadCachePolicyType
{
    ReadAhead,
    AdaptiveReadAhead,
    Off,
};
enum class VolumeV1VolumeType
{
    RawDevice,
    NonRedundant,
    Mirrored,
    StripedWithParity,
    SpannedMirrors,
    SpannedStripesWithParity,
};
enum class VolumeV1VolumeUsageType
{
    Data,
    SystemData,
    CacheOnly,
    SystemReserve,
    ReplicationReserve,
};
enum class VolumeV1WriteCachePolicyType
{
    WriteThrough,
    ProtectedWriteBack,
    UnprotectedWriteBack,
    Off,
};
enum class VolumeV1WriteCacheStateType
{
    Unprotected,
    Protected,
    Degraded,
};
enum class VolumeV1WriteHoleProtectionPolicyType
{
    Off,
    Journaling,
    DistributedLog,
    Oem,
};
struct VolumeV1OemActions
{};
struct VolumeV1Actions
{
    VolumeV1OemActions oem;
};
struct VolumeV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ drives;
    NavigationReference_ classOfService;
    NavigationReference_ dedicatedSpareDrives;
    NavigationReference_ spareResourceSets;
    NavigationReference_ clientEndpoints;
    NavigationReference_ serverEndpoints;
    NavigationReference_ consistencyGroups;
    NavigationReference_ owningStorageService;
    NavigationReference_ journalingMedia;
    NavigationReference_ owningStorageResource;
    NavigationReference_ cacheVolumeSource;
    NavigationReference_ cacheDataVolumes;
};
struct VolumeV1NamespaceFeatures
{
    bool supportsThinProvisioning;
    bool supportsDeallocatedOrUnwrittenLBError;
    bool supportsNGUIDReuse;
    bool supportsAtomicTransactionSize;
    bool supportsIOPerformanceHints;
};
struct VolumeV1NVMeNamespaceProperties
{
    std::string namespaceId;
    bool isShareable;
    VolumeV1NamespaceFeatures namespaceFeatures;
    int64_t numberLBAFormats;
    std::string formattedLBASize;
    bool metadataTransferredAtEndOfDataLBA;
    std::string nVMeVersion;
};
struct VolumeV1Operation
{
    std::string operationName;
    int64_t percentageComplete;
    NavigationReference_ associatedFeaturesRegistry;
};
struct VolumeV1Volume
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    int64_t capacityBytes;
    VolumeV1VolumeType volumeType;
    bool encrypted;
    VolumeV1EncryptionTypes encryptionTypes;
    ResourceV1Resource identifiers;
    int64_t blockSizeBytes;
    VolumeV1Operation operations;
    int64_t optimumIOSizeBytes;
    VolumeV1Links links;
    VolumeV1Actions actions;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities accessCapabilities;
    int64_t maxBlockSizeBytes;
    CapacityV1Capacity capacity;
    int64_t lowSpaceWarningThresholdPercents;
    std::string manufacturer;
    std::string model;
    StorageReplicaInfoV1StorageReplicaInfo replicaInfo;
    IOStatisticsV1IOStatistics iOStatistics;
    int64_t remainingCapacityPercent;
    int64_t recoverableCapacitySourceCount;
    NavigationReference_ replicaTargets;
    VolumeV1RAIDType rAIDType;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities provisioningPolicy;
    int64_t stripSizeBytes;
    VolumeV1ReadCachePolicyType readCachePolicy;
    VolumeV1VolumeUsageType volumeUsage;
    VolumeV1WriteCachePolicyType writeCachePolicy;
    VolumeV1WriteCacheStateType writeCacheState;
    int64_t logicalUnitNumber;
    int64_t mediaSpanCount;
    std::string displayName;
    VolumeV1WriteHoleProtectionPolicyType writeHoleProtectionPolicy;
    bool deduplicated;
    bool compressed;
    bool iOPerfModeEnabled;
    VolumeV1NVMeNamespaceProperties nVMeNamespaceProperties;
    VolumeV1InitializeMethod initializeMethod;
};
#endif
