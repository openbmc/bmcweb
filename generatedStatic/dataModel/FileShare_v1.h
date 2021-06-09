#ifndef FILESHARE_V1
#define FILESHARE_V1

#include "DataStorageLoSCapabilities_v1.h"
#include "EthernetInterfaceCollection_v1.h"
#include "FileShare_v1.h"
#include "FileSystem_v1.h"
#include "NavigationReferenceRedfish.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class FileShareV1QuotaType
{
    Soft,
    Hard,
};
struct FileShareV1OemActions
{};
struct FileShareV1Actions
{
    FileShareV1OemActions oem;
};
struct FileShareV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish classOfService;
    NavigationReferenceRedfish fileSystem;
};
struct FileShareV1FileShare
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    std::string fileSharePath;
    FileSystemV1FileSystem fileSharingProtocols;
    ResourceV1Resource status;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities
        defaultAccessCapabilities;
    bool executeSupport;
    bool rootAccess;
    StorageReplicaInfoV1StorageReplicaInfo writePolicy;
    bool cASupported;
    int64_t fileShareTotalQuotaBytes;
    int64_t fileShareRemainingQuotaBytes;
    int64_t lowSpaceWarningThresholdPercents;
    FileShareV1QuotaType fileShareQuotaType;
    FileShareV1Links links;
    EthernetInterfaceCollectionV1EthernetInterfaceCollection ethernetInterfaces;
    int64_t remainingCapacityPercent;
    FileShareV1Actions actions;
};
#endif
