#ifndef DATASTORAGELINEOFSERVICE_V1
#define DATASTORAGELINEOFSERVICE_V1

#include "DataProtectionLoSCapabilities_v1.h"
#include "DataStorageLineOfService_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "Resource_v1.h"

struct DataStorageLineOfServiceV1OemActions
{};
struct DataStorageLineOfServiceV1Actions
{
    DataStorageLineOfServiceV1OemActions oem;
};
struct DataStorageLineOfServiceV1DataStorageLineOfService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    DataProtectionLoSCapabilitiesV1DataProtectionLoSCapabilities
        recoveryTimeObjectives;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities provisioningPolicy;
    bool isSpaceEfficient;
    DataStorageLoSCapabilitiesV1DataStorageLoSCapabilities accessCapabilities;
    int64_t recoverableCapacitySourceCount;
    DataStorageLineOfServiceV1Actions actions;
};
#endif
