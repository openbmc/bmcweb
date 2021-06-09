#ifndef FILESHARE_V1
#define FILESHARE_V1

#include "DataStorageLoSCapabilities_v1.h"
#include "EthernetInterfaceCollection_v1.h"
#include "FileShare_v1.h"
#include "FileSystem_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class FileShare_v1_QuotaType
{
    Soft,
    Hard,
};
struct FileShare_v1_Actions
{
    FileShare_v1_OemActions oem;
};
struct FileShare_v1_FileShare
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string fileSharePath;
    FileSystem_v1_FileSystem fileSharingProtocols;
    Resource_v1_Resource status;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities
        defaultAccessCapabilities;
    bool executeSupport;
    bool rootAccess;
    StorageReplicaInfo_v1_StorageReplicaInfo writePolicy;
    bool cASupported;
    int64_t fileShareTotalQuotaBytes;
    int64_t fileShareRemainingQuotaBytes;
    int64_t lowSpaceWarningThresholdPercents;
    FileShare_v1_QuotaType fileShareQuotaType;
    FileShare_v1_Links links;
    EthernetInterfaceCollection_v1_EthernetInterfaceCollection
        ethernetInterfaces;
    int64_t remainingCapacityPercent;
    FileShare_v1_Actions actions;
};
struct FileShare_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ classOfService;
    NavigationReference__ fileSystem;
};
struct FileShare_v1_OemActions
{};
#endif
