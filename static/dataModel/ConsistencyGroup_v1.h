#ifndef CONSISTENCYGROUP_V1
#define CONSISTENCYGROUP_V1

#include "ConsistencyGroup_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class ConsistencyGroup_v1_ApplicationConsistencyMethod
{
    HotStandby,
    VASA,
    VDI,
    VSS,
    Other,
};
enum class ConsistencyGroup_v1_ConsistencyType
{
    CrashConsistent,
    ApplicationConsistent,
};
struct ConsistencyGroup_v1_Actions
{
    ConsistencyGroup_v1_OemActions oem;
};
struct ConsistencyGroup_v1_ConsistencyGroup
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool isConsistent;
    ConsistencyGroup_v1_ApplicationConsistencyMethod consistencyMethod;
    ConsistencyGroup_v1_ConsistencyType consistencyType;
    Resource_v1_Resource status;
    StorageReplicaInfo_v1_StorageReplicaInfo replicaInfo;
    NavigationReference__ replicaTargets;
    NavigationReference__ volumes;
    ConsistencyGroup_v1_Links links;
    ConsistencyGroup_v1_Actions actions;
};
struct ConsistencyGroup_v1_Links
{
    Resource_v1_Resource oem;
};
struct ConsistencyGroup_v1_OemActions
{};
#endif
