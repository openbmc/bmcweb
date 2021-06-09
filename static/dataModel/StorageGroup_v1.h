#ifndef STORAGEGROUP_V1
#define STORAGEGROUP_V1

#include "EndpointGroup_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StorageGroup_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class StorageGroup_v1_AccessCapability {
    Read,
    ReadWrite,
};
enum class StorageGroup_v1_AuthenticationMethod {
    None,
    CHAP,
    MutualCHAP,
    DHCHAP,
};
struct StorageGroup_v1_Actions
{
    StorageGroup_v1_OemActions oem;
};
struct StorageGroup_v1_CHAPInformation
{
    std::string initiatorCHAPUser;
    std::string initiatorCHAPPassword;
    std::string targetCHAPUser;
    std::string targetPassword;
    std::string cHAPUser;
    std::string cHAPPassword;
    std::string targetCHAPPassword;
};
struct StorageGroup_v1_DHCHAPInformation
{
    std::string localDHCHAPAuthSecret;
    std::string peerDHCHAPAuthSecret;
};
struct StorageGroup_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ parentStorageGroups;
    NavigationReference__ childStorageGroups;
    NavigationReference__ classOfService;
};
struct StorageGroup_v1_MappedVolume
{
    std::string logicalUnitNumber;
    NavigationReference__ volume;
    StorageGroup_v1_AccessCapability accessCapability;
};
struct StorageGroup_v1_OemActions
{
};
struct StorageGroup_v1_StorageGroup
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    EndpointGroup_v1_EndpointGroup accessState;
    bool membersAreConsistent;
    bool volumesAreExposed;
    Resource_v1_Resource status;
    StorageGroup_v1_Links links;
    StorageReplicaInfo_v1_StorageReplicaInfo replicaInfo;
    EndpointGroup_v1_EndpointGroup clientEndpointGroups;
    EndpointGroup_v1_EndpointGroup serverEndpointGroups;
    NavigationReference__ volumes;
    StorageGroup_v1_Actions actions;
    StorageGroup_v1_MappedVolume mappedVolumes;
    NavigationReference__ replicaTargets;
    StorageGroup_v1_AuthenticationMethod authenticationMethod;
    StorageGroup_v1_CHAPInformation chapInfo;
    StorageGroup_v1_DHCHAPInformation dHChapInfo;
};
#endif
