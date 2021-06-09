#ifndef FILESYSTEM_V1
#define FILESYSTEM_V1

#include "Capacity_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "FileShareCollection_v1.h"
#include "FileSystem_v1.h"
#include "IOStatistics_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class FileSystemV1CharacterCodeSet
{
    ASCII,
    Unicode,
    ISO2022,
    ISO8859_1,
    ExtendedUNIXCode,
    UTF_8,
    UTF_16,
    UCS_2,
};
enum class FileSystemV1FileProtocol
{
    NFSv3,
    NFSv4_0,
    NFSv4_1,
    SMBv2_0,
    SMBv2_1,
    SMBv3_0,
    SMBv3_0_2,
    SMBv3_1_1,
};
struct FileSystemV1OemActions
{};
struct FileSystemV1Actions
{
    FileSystemV1OemActions oem;
};
struct FileSystemV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ replicaCollection;
    NavigationReference_ classOfService;
    NavigationReference_ spareResourceSets;
};
struct FileSystemV1ImportedShare
{
    NavigationReference_ importedShare;
    std::string fileSharePath;
};
struct FileSystemV1FileSystem
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t blockSizeBytes;
    CapacityV1Capacity capacity;
    CapacityV1Capacity remainingCapacity;
    int64_t lowSpaceWarningThresholdPercents;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities accessCapabilities;
    bool caseSensitive;
    bool casePreserved;
    FileSystemV1CharacterCodeSet characterCodeSet;
    int64_t maxFileNameLengthBytes;
    int64_t clusterSizeBytes;
    StorageReplicaInfoV1StorageReplicaInfo replicaInfo;
    FileShareCollectionV1FileShareCollection exportedShares;
    FileSystemV1Links links;
    FileSystemV1ImportedShare importedShares;
    int64_t remainingCapacityPercent;
    FileSystemV1Actions actions;
    ResourceV1Resource identifiers;
    IOStatisticsV1IOStatistics iOStatistics;
    int64_t recoverableCapacitySourceCount;
    NavigationReference_ replicaTargets;
};
#endif
