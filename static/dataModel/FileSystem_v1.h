#ifndef FILESYSTEM_V1
#define FILESYSTEM_V1

#include "Capacity_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "FileShareCollection_v1.h"
#include "FileSystem_v1.h"
#include "IOStatistics_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class FileSystem_v1_CharacterCodeSet {
    ASCII,
    Unicode,
    ISO2022,
    ISO8859_1,
    ExtendedUNIXCode,
    UTF_8,
    UTF_16,
    UCS_2,
};
enum class FileSystem_v1_FileProtocol {
    NFSv3,
    NFSv4_0,
    NFSv4_1,
    SMBv2_0,
    SMBv2_1,
    SMBv3_0,
    SMBv3_0_2,
    SMBv3_1_1,
};
struct FileSystem_v1_Actions
{
    FileSystem_v1_OemActions oem;
};
struct FileSystem_v1_FileSystem
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    int64_t blockSizeBytes;
    Capacity_v1_Capacity capacity;
    Capacity_v1_Capacity remainingCapacity;
    int64_t lowSpaceWarningThresholdPercents;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities accessCapabilities;
    bool caseSensitive;
    bool casePreserved;
    FileSystem_v1_CharacterCodeSet characterCodeSet;
    int64_t maxFileNameLengthBytes;
    int64_t clusterSizeBytes;
    StorageReplicaInfo_v1_StorageReplicaInfo replicaInfo;
    FileShareCollection_v1_FileShareCollection exportedShares;
    FileSystem_v1_Links links;
    FileSystem_v1_ImportedShare importedShares;
    int64_t remainingCapacityPercent;
    FileSystem_v1_Actions actions;
    Resource_v1_Resource identifiers;
    IOStatistics_v1_IOStatistics iOStatistics;
    int64_t recoverableCapacitySourceCount;
    NavigationReference__ replicaTargets;
};
struct FileSystem_v1_ImportedShare
{
    NavigationReference__ importedShare;
    std::string fileSharePath;
};
struct FileSystem_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ replicaCollection;
    NavigationReference__ classOfService;
    NavigationReference__ spareResourceSets;
};
struct FileSystem_v1_OemActions
{
};
#endif
