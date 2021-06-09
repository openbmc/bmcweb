#ifndef DATAPROTECTIONLINEOFSERVICE_V1
#define DATAPROTECTIONLINEOFSERVICE_V1

#include "DataProtectionLineOfService_v1.h"
#include "DataProtectionLoSCapabilities_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"
#include "StorageReplicaInfo_v1.h"

struct DataProtectionLineOfService_v1_Actions
{
    DataProtectionLineOfService_v1_OemActions oem;
};
struct DataProtectionLineOfService_v1_DataProtectionLineOfService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    DataProtectionLoSCapabilities_v1_DataProtectionLoSCapabilities recoveryGeographicObjective;
    std::string recoveryPointObjectiveTime;
    DataProtectionLoSCapabilities_v1_DataProtectionLoSCapabilities recoveryTimeObjective;
    StorageReplicaInfo_v1_StorageReplicaInfo replicaType;
    std::string minLifetime;
    bool isIsolated;
    Schedule_v1_Schedule schedule;
    NavigationReference__ replicaClassOfService;
    Resource_v1_Resource replicaAccessLocation;
    DataProtectionLineOfService_v1_Actions actions;
};
struct DataProtectionLineOfService_v1_OemActions
{
};
struct DataProtectionLineOfService_v1_ReplicaRequest
{
    NavigationReference__ replicaSource;
    std::string replicaName;
};
#endif
