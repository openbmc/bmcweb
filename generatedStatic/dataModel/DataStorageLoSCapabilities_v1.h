#ifndef DATASTORAGELOSCAPABILITIES_V1
#define DATASTORAGELOSCAPABILITIES_V1

#include "DataProtectionLoSCapabilities_v1.h"
#include "DataStorageLineOfService_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "Resource_v1.h"

enum class DataStorageLoSCapabilitiesV1ProvisioningPolicy
{
    Fixed,
    Thin,
};
enum class DataStorageLoSCapabilitiesV1StorageAccessCapability
{
    Read,
    Write,
    WriteOnce,
    Append,
    Streaming,
    Execute,
};
struct DataStorageLoSCapabilitiesV1OemActions
{};
struct DataStorageLoSCapabilitiesV1Actions
{
    DataStorageLoSCapabilitiesV1OemActions oem;
};
struct DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    DataStorageLoSCapabilitiesV1StorageAccessCapability
        supportedAccessCapabilities;
    DataProtectionLoSCapabilitiesV1DataProtectionLoSCapabilities
        supportedRecoveryTimeObjectives;
    DataStorageLoSCapabilitiesV1ProvisioningPolicy
        supportedProvisioningPolicies;
    bool supportsSpaceEfficiency;
    DataStorageLineOfServiceV1DataStorageLineOfService supportedLinesOfService;
    DataStorageLoSCapabilitiesV1Actions actions;
    int64_t maximumRecoverableCapacitySourceCount;
};
#endif
