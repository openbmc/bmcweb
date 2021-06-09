#ifndef DATAPROTECTIONLINEOFSERVICE_V1
#define DATAPROTECTIONLINEOFSERVICE_V1

#include "DataProtectionLineOfService_v1.h"
#include "DataProtectionLoSCapabilities_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"
#include "StorageReplicaInfo_v1.h"

struct DataProtectionLineOfServiceV1OemActions
{};
struct DataProtectionLineOfServiceV1Actions
{
    DataProtectionLineOfServiceV1OemActions oem;
};
struct DataProtectionLineOfServiceV1DataProtectionLineOfService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    DataProtectionLoSCapabilitiesV1DataProtectionLoSCapabilities
        recoveryGeographicObjective;
    std::string recoveryPointObjectiveTime;
    DataProtectionLoSCapabilitiesV1DataProtectionLoSCapabilities
        recoveryTimeObjective;
    StorageReplicaInfoV1StorageReplicaInfo replicaType;
    std::string minLifetime;
    bool isIsolated;
    ScheduleV1Schedule schedule;
    NavigationReference_ replicaClassOfService;
    ResourceV1Resource replicaAccessLocation;
    DataProtectionLineOfServiceV1Actions actions;
};
struct DataProtectionLineOfServiceV1ReplicaRequest
{
    NavigationReference_ replicaSource;
    std::string replicaName;
};
#endif
