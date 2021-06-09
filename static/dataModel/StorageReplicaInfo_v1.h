#ifndef STORAGEREPLICAINFO_V1
#define STORAGEREPLICAINFO_V1

#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class StorageReplicaInfo_v1_ConsistencyState {
    Consistent,
    Inconsistent,
};
enum class StorageReplicaInfo_v1_ConsistencyStatus {
    Consistent,
    InProgress,
    Disabled,
    InError,
};
enum class StorageReplicaInfo_v1_ConsistencyType {
    SequentiallyConsistent,
};
enum class StorageReplicaInfo_v1_ReplicaFaultDomain {
    Local,
    Remote,
};
enum class StorageReplicaInfo_v1_ReplicaPriority {
    Low,
    Same,
    High,
    Urgent,
};
enum class StorageReplicaInfo_v1_ReplicaProgressStatus {
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
enum class StorageReplicaInfo_v1_ReplicaReadOnlyAccess {
    SourceElement,
    ReplicaElement,
    Both,
};
enum class StorageReplicaInfo_v1_ReplicaRecoveryMode {
    Automatic,
    Manual,
};
enum class StorageReplicaInfo_v1_ReplicaRole {
    Source,
    Target,
};
enum class StorageReplicaInfo_v1_ReplicaState {
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
enum class StorageReplicaInfo_v1_ReplicaType {
    Mirror,
    Snapshot,
    Clone,
    TokenizedClone,
};
enum class StorageReplicaInfo_v1_ReplicaUpdateMode {
    Active,
    Synchronous,
    Asynchronous,
    Adaptive,
};
enum class StorageReplicaInfo_v1_UndiscoveredElement {
    SourceElement,
    ReplicaElement,
};
struct StorageReplicaInfo_v1_Actions
{
    StorageReplicaInfo_v1_OemActions oem;
};
struct StorageReplicaInfo_v1_OemActions
{
};
struct StorageReplicaInfo_v1_ReplicaInfo
{
    StorageReplicaInfo_v1_ReplicaPriority replicaPriority;
    StorageReplicaInfo_v1_ReplicaReadOnlyAccess replicaReadOnlyAccess;
    StorageReplicaInfo_v1_UndiscoveredElement undiscoveredElement;
    std::string whenSynced;
    bool syncMaintained;
    StorageReplicaInfo_v1_ReplicaRecoveryMode replicaRecoveryMode;
    StorageReplicaInfo_v1_ReplicaUpdateMode replicaUpdateMode;
    int64_t percentSynced;
    bool failedCopyStopsHostIO;
    std::string whenActivated;
    std::string whenDeactivated;
    std::string whenEstablished;
    std::string whenSuspended;
    std::string whenSynchronized;
    int64_t replicaSkewBytes;
    StorageReplicaInfo_v1_ReplicaType replicaType;
    StorageReplicaInfo_v1_ReplicaProgressStatus replicaProgressStatus;
    StorageReplicaInfo_v1_ReplicaState replicaState;
    StorageReplicaInfo_v1_ReplicaState requestedReplicaState;
    bool consistencyEnabled;
    StorageReplicaInfo_v1_ConsistencyType consistencyType;
    StorageReplicaInfo_v1_ConsistencyState consistencyState;
    StorageReplicaInfo_v1_ConsistencyStatus consistencyStatus;
    StorageReplicaInfo_v1_ReplicaRole replicaRole;
    NavigationReference__ replica;
    NavigationReference__ dataProtectionLineOfService;
    NavigationReference__ sourceReplica;
    StorageReplicaInfo_v1_ReplicaFaultDomain replicaFaultDomain;
};
struct StorageReplicaInfo_v1_StorageReplicaInfo
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    StorageReplicaInfo_v1_Actions actions;
};
#endif
