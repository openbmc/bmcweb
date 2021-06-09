#ifndef STORAGEGROUP_V1
#define STORAGEGROUP_V1

#include "EndpointGroup_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "StorageGroup_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class StorageGroupV1AccessCapability
{
    Read,
    ReadWrite,
};
enum class StorageGroupV1AuthenticationMethod
{
    None,
    CHAP,
    MutualCHAP,
    DHCHAP,
};
struct StorageGroupV1OemActions
{};
struct StorageGroupV1Actions
{
    StorageGroupV1OemActions oem;
};
struct StorageGroupV1CHAPInformation
{
    std::string initiatorCHAPUser;
    std::string initiatorCHAPPassword;
    std::string targetCHAPUser;
    std::string targetPassword;
    std::string cHAPUser;
    std::string cHAPPassword;
    std::string targetCHAPPassword;
};
struct StorageGroupV1DHCHAPInformation
{
    std::string localDHCHAPAuthSecret;
    std::string peerDHCHAPAuthSecret;
};
struct StorageGroupV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ parentStorageGroups;
    NavigationReference_ childStorageGroups;
    NavigationReference_ classOfService;
};
struct StorageGroupV1MappedVolume
{
    std::string logicalUnitNumber;
    NavigationReference_ volume;
    StorageGroupV1AccessCapability accessCapability;
};
struct StorageGroupV1StorageGroup
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    EndpointGroupV1EndpointGroup accessState;
    bool membersAreConsistent;
    bool volumesAreExposed;
    ResourceV1Resource status;
    StorageGroupV1Links links;
    StorageReplicaInfoV1StorageReplicaInfo replicaInfo;
    EndpointGroupV1EndpointGroup clientEndpointGroups;
    EndpointGroupV1EndpointGroup serverEndpointGroups;
    NavigationReference_ volumes;
    StorageGroupV1Actions actions;
    StorageGroupV1MappedVolume mappedVolumes;
    NavigationReference_ replicaTargets;
    StorageGroupV1AuthenticationMethod authenticationMethod;
    StorageGroupV1CHAPInformation chapInfo;
    StorageGroupV1DHCHAPInformation dHChapInfo;
};
#endif
