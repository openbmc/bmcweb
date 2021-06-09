#ifndef VOLUME_V1
#define VOLUME_V1

#include "Capacity_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "IOStatistics_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"
#include "Volume_v1.h"

enum class Volume_v1_EncryptionTypes {
    NativeDriveEncryption,
    ControllerAssisted,
    SoftwareAssisted,
};
enum class Volume_v1_InitializeMethod {
    Skip,
    Background,
    Foreground,
};
enum class Volume_v1_InitializeType {
    Fast,
    Slow,
};
enum class Volume_v1_RAIDType {
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
enum class Volume_v1_ReadCachePolicyType {
    ReadAhead,
    AdaptiveReadAhead,
    Off,
};
enum class Volume_v1_VolumeType {
    RawDevice,
    NonRedundant,
    Mirrored,
    StripedWithParity,
    SpannedMirrors,
    SpannedStripesWithParity,
};
enum class Volume_v1_VolumeUsageType {
    Data,
    SystemData,
    CacheOnly,
    SystemReserve,
    ReplicationReserve,
};
enum class Volume_v1_WriteCachePolicyType {
    WriteThrough,
    ProtectedWriteBack,
    UnprotectedWriteBack,
    Off,
};
enum class Volume_v1_WriteCacheStateType {
    Unprotected,
    Protected,
    Degraded,
};
enum class Volume_v1_WriteHoleProtectionPolicyType {
    Off,
    Journaling,
    DistributedLog,
    Oem,
};
struct Volume_v1_Actions
{
    Volume_v1_OemActions oem;
};
struct Volume_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ drives;
    NavigationReference__ classOfService;
    NavigationReference__ dedicatedSpareDrives;
    NavigationReference__ spareResourceSets;
    NavigationReference__ clientEndpoints;
    NavigationReference__ serverEndpoints;
    NavigationReference__ consistencyGroups;
    NavigationReference__ owningStorageService;
    NavigationReference__ journalingMedia;
    NavigationReference__ owningStorageResource;
};
struct Volume_v1_NamespaceFeatures
{
    bool supportsThinProvisioning;
    bool supportsDeallocatedOrUnwrittenLBError;
    bool supportsNGUIDReuse;
    bool supportsAtomicTransactionSize;
    bool supportsIOPerformanceHints;
};
struct Volume_v1_NVMeNamespaceProperties
{
    std::string namespaceId;
    bool isShareable;
    Volume_v1_NamespaceFeatures namespaceFeatures;
    int64_t numberLBAFormats;
    std::string formattedLBASize;
    bool metadataTransferredAtEndOfDataLBA;
    std::string nVMeVersion;
};
struct Volume_v1_OemActions
{
};
struct Volume_v1_Operation
{
    std::string operationName;
    int64_t percentageComplete;
    NavigationReference__ associatedFeaturesRegistry;
};
struct Volume_v1_Volume
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    int64_t capacityBytes;
    Volume_v1_VolumeType volumeType;
    bool encrypted;
    Volume_v1_EncryptionTypes encryptionTypes;
    Resource_v1_Resource identifiers;
    int64_t blockSizeBytes;
    Volume_v1_Operation operations;
    int64_t optimumIOSizeBytes;
    Volume_v1_Links links;
    Volume_v1_Actions actions;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities accessCapabilities;
    int64_t maxBlockSizeBytes;
    Capacity_v1_Capacity capacity;
    int64_t lowSpaceWarningThresholdPercents;
    std::string manufacturer;
    std::string model;
    StorageReplicaInfo_v1_StorageReplicaInfo replicaInfo;
    IOStatistics_v1_IOStatistics iOStatistics;
    int64_t remainingCapacityPercent;
    int64_t recoverableCapacitySourceCount;
    NavigationReference__ replicaTargets;
    Volume_v1_RAIDType rAIDType;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities provisioningPolicy;
    int64_t stripSizeBytes;
    Volume_v1_ReadCachePolicyType readCachePolicy;
    Volume_v1_VolumeUsageType volumeUsage;
    Volume_v1_WriteCachePolicyType writeCachePolicy;
    Volume_v1_WriteCacheStateType writeCacheState;
    int64_t logicalUnitNumber;
    int64_t mediaSpanCount;
    std::string displayName;
    Volume_v1_WriteHoleProtectionPolicyType writeHoleProtectionPolicy;
    bool deduplicated;
    bool compressed;
    bool iOPerfModeEnabled;
    Volume_v1_NVMeNamespaceProperties nVMeNamespaceProperties;
};
#endif
