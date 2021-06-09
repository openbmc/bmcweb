#ifndef STORAGEREPLICAINFO_V1
#define STORAGEREPLICAINFO_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class StorageReplicaInfoV1ConsistencyState
{
    Consistent,
    Inconsistent,
};
enum class StorageReplicaInfoV1ConsistencyStatus
{
    Consistent,
    InProgress,
    Disabled,
    InError,
};
enum class StorageReplicaInfoV1ConsistencyType
{
    SequentiallyConsistent,
};
enum class StorageReplicaInfoV1ReplicaFaultDomain
{
    Local,
    Remote,
};
enum class StorageReplicaInfoV1ReplicaPriority
{
    Low,
    Same,
    High,
    Urgent,
};
enum class StorageReplicaInfoV1ReplicaProgressStatus
{
    Completed,
    Dormant,
    Initializing,
    Preparing,
    Synchronizing,
    Resyncing,
    Restoring,
    Fracturing,
    Splitting,
    FailingOver,
    FailingBack,
    Detaching,
    Aborting,
    Mixed,
    Suspending,
    RequiresFracture,
    RequiresResync,
    RequiresActivate,
    Pending,
    RequiresDetach,
    Terminating,
    RequiresSplit,
    RequiresResume,
};
enum class StorageReplicaInfoV1ReplicaReadOnlyAccess
{
    SourceElement,
    ReplicaElement,
    Both,
};
enum class StorageReplicaInfoV1ReplicaRecoveryMode
{
    Automatic,
    Manual,
};
enum class StorageReplicaInfoV1ReplicaRole
{
    Source,
    Target,
};
enum class StorageReplicaInfoV1ReplicaState
{
    Initialized,
    Unsynchronized,
    Synchronized,
    Broken,
    Fractured,
    Split,
    Inactive,
    Suspended,
    Failedover,
    Prepared,
    Aborted,
    Skewed,
    Mixed,
    Partitioned,
    Invalid,
    Restored,
};
enum class StorageReplicaInfoV1ReplicaType
{
    Mirror,
    Snapshot,
    Clone,
    TokenizedClone,
};
enum class StorageReplicaInfoV1ReplicaUpdateMode
{
    Active,
    Synchronous,
    Asynchronous,
    Adaptive,
};
enum class StorageReplicaInfoV1UndiscoveredElement
{
    SourceElement,
    ReplicaElement,
};
struct StorageReplicaInfoV1OemActions
{};
struct StorageReplicaInfoV1Actions
{
    StorageReplicaInfoV1OemActions oem;
};
struct StorageReplicaInfoV1ReplicaInfo
{
    StorageReplicaInfoV1ReplicaPriority replicaPriority;
    StorageReplicaInfoV1ReplicaReadOnlyAccess replicaReadOnlyAccess;
    StorageReplicaInfoV1UndiscoveredElement undiscoveredElement;
    std::string whenSynced;
    bool syncMaintained;
    StorageReplicaInfoV1ReplicaRecoveryMode replicaRecoveryMode;
    StorageReplicaInfoV1ReplicaUpdateMode replicaUpdateMode;
    int64_t percentSynced;
    bool failedCopyStopsHostIO;
    std::string whenActivated;
    std::string whenDeactivated;
    std::string whenEstablished;
    std::string whenSuspended;
    std::string whenSynchronized;
    int64_t replicaSkewBytes;
    StorageReplicaInfoV1ReplicaType replicaType;
    StorageReplicaInfoV1ReplicaProgressStatus replicaProgressStatus;
    StorageReplicaInfoV1ReplicaState replicaState;
    StorageReplicaInfoV1ReplicaState requestedReplicaState;
    bool consistencyEnabled;
    StorageReplicaInfoV1ConsistencyType consistencyType;
    StorageReplicaInfoV1ConsistencyState consistencyState;
    StorageReplicaInfoV1ConsistencyStatus consistencyStatus;
    StorageReplicaInfoV1ReplicaRole replicaRole;
    NavigationReference_ replica;
    NavigationReference_ dataProtectionLineOfService;
    NavigationReference_ sourceReplica;
    StorageReplicaInfoV1ReplicaFaultDomain replicaFaultDomain;
};
struct StorageReplicaInfoV1StorageReplicaInfo
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    StorageReplicaInfoV1Actions actions;
};
#endif
