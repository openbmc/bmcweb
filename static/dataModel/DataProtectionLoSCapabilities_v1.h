#ifndef DATAPROTECTIONLOSCAPABILITIES_V1
#define DATAPROTECTIONLOSCAPABILITIES_V1

#include "DataProtectionLineOfService_v1.h"
#include "DataProtectionLoSCapabilities_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "StorageReplicaInfo_v1.h"

enum class DataProtectionLoSCapabilitiesV1FailureDomainScope
{
    Server,
    Rack,
    RackGroup,
    Row,
    Datacenter,
    Region,
};
enum class DataProtectionLoSCapabilitiesV1RecoveryAccessScope
{
    OnlineActive,
    OnlinePassive,
    Nearline,
    Offline,
};
struct DataProtectionLoSCapabilitiesV1OemActions
{};
struct DataProtectionLoSCapabilitiesV1Actions
{
    DataProtectionLoSCapabilitiesV1OemActions oem;
};
struct DataProtectionLoSCapabilitiesV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ supportedReplicaOptions;
};
struct DataProtectionLoSCapabilitiesV1DataProtectionLoSCapabilities
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    DataProtectionLoSCapabilitiesV1FailureDomainScope
        supportedRecoveryGeographicObjectives;
    std::string supportedRecoveryPointObjectiveTimes;
    DataProtectionLoSCapabilitiesV1RecoveryAccessScope
        supportedRecoveryTimeObjectives;
    StorageReplicaInfoV1StorageReplicaInfo supportedReplicaTypes;
    std::string supportedMinLifetimes;
    bool supportsIsolated;
    DataProtectionLoSCapabilitiesV1Links links;
    DataProtectionLineOfServiceV1DataProtectionLineOfService
        supportedLinesOfService;
    DataProtectionLoSCapabilitiesV1Actions actions;
};
#endif
