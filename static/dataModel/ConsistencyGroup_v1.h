#ifndef CONSISTENCYGROUP_V1
#define CONSISTENCYGROUP_V1

#include "ConsistencyGroup_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class ConsistencyGroupV1ApplicationConsistencyMethod
{
    HotStandby,
    VASA,
    VDI,
    VSS,
    Other,
};
enum class ConsistencyGroupV1ConsistencyType
{
    CrashConsistent,
    ApplicationConsistent,
};
struct ConsistencyGroupV1OemActions
{};
struct ConsistencyGroupV1Actions
{
    ConsistencyGroupV1OemActions oem;
};
struct ConsistencyGroupV1Links
{
    ResourceV1Resource oem;
};
struct ConsistencyGroupV1ConsistencyGroup
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool isConsistent;
    ConsistencyGroupV1ApplicationConsistencyMethod consistencyMethod;
    ConsistencyGroupV1ConsistencyType consistencyType;
    ResourceV1Resource status;
    StorageReplicaInfoV1StorageReplicaInfo replicaInfo;
    NavigationReference_ replicaTargets;
    NavigationReference_ volumes;
    ConsistencyGroupV1Links links;
    ConsistencyGroupV1Actions actions;
};
#endif
