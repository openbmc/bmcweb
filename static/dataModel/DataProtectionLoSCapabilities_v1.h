#ifndef DATAPROTECTIONLOSCAPABILITIES_V1
#define DATAPROTECTIONLOSCAPABILITIES_V1

#include "DataProtectionLineOfService_v1.h"
#include "DataProtectionLoSCapabilities_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class DataProtectionLoSCapabilities_v1_FailureDomainScope
{
    Server,
    Rack,
    RackGroup,
    Row,
    Datacenter,
    Region,
};
enum class DataProtectionLoSCapabilities_v1_RecoveryAccessScope
{
    OnlineActive,
    OnlinePassive,
    Nearline,
    Offline,
};
struct DataProtectionLoSCapabilities_v1_Actions
{
    DataProtectionLoSCapabilities_v1_OemActions oem;
};
struct DataProtectionLoSCapabilities_v1_DataProtectionLoSCapabilities
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    DataProtectionLoSCapabilities_v1_FailureDomainScope
        supportedRecoveryGeographicObjectives;
    std::string supportedRecoveryPointObjectiveTimes;
    DataProtectionLoSCapabilities_v1_RecoveryAccessScope
        supportedRecoveryTimeObjectives;
    StorageReplicaInfo_v1_StorageReplicaInfo supportedReplicaTypes;
    std::string supportedMinLifetimes;
    bool supportsIsolated;
    DataProtectionLoSCapabilities_v1_Links links;
    DataProtectionLineOfService_v1_DataProtectionLineOfService
        supportedLinesOfService;
    DataProtectionLoSCapabilities_v1_Actions actions;
};
struct DataProtectionLoSCapabilities_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ supportedReplicaOptions;
};
struct DataProtectionLoSCapabilities_v1_OemActions
{};
#endif
