#ifndef DATASTORAGELOSCAPABILITIES_V1
#define DATASTORAGELOSCAPABILITIES_V1

#include "DataProtectionLoSCapabilities_v1.h"
#include "DataStorageLineOfService_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "Resource_v1.h"

enum class DataStorageLoSCapabilities_v1_ProvisioningPolicy
{
    Fixed,
    Thin,
};
enum class DataStorageLoSCapabilities_v1_StorageAccessCapability
{
    Read,
    Write,
    WriteOnce,
    Append,
    Streaming,
    Execute,
};
struct DataStorageLoSCapabilities_v1_Actions
{
    DataStorageLoSCapabilities_v1_OemActions oem;
};
struct DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    DataStorageLoSCapabilities_v1_StorageAccessCapability
        supportedAccessCapabilities;
    DataProtectionLoSCapabilities_v1_DataProtectionLoSCapabilities
        supportedRecoveryTimeObjectives;
    DataStorageLoSCapabilities_v1_ProvisioningPolicy
        supportedProvisioningPolicies;
    bool supportsSpaceEfficiency;
    DataStorageLineOfService_v1_DataStorageLineOfService
        supportedLinesOfService;
    DataStorageLoSCapabilities_v1_Actions actions;
    int64_t maximumRecoverableCapacitySourceCount;
};
struct DataStorageLoSCapabilities_v1_OemActions
{};
#endif
