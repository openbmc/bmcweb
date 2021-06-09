#ifndef DATASTORAGELINEOFSERVICE_V1
#define DATASTORAGELINEOFSERVICE_V1

#include "DataProtectionLoSCapabilities_v1.h"
#include "DataStorageLineOfService_v1.h"
#include "DataStorageLoSCapabilities_v1.h"
#include "Resource_v1.h"

struct DataStorageLineOfService_v1_Actions
{
    DataStorageLineOfService_v1_OemActions oem;
};
struct DataStorageLineOfService_v1_DataStorageLineOfService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    DataProtectionLoSCapabilities_v1_DataProtectionLoSCapabilities
        recoveryTimeObjectives;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities provisioningPolicy;
    bool isSpaceEfficient;
    DataStorageLoSCapabilities_v1_DataStorageLoSCapabilities accessCapabilities;
    int64_t recoverableCapacitySourceCount;
    DataStorageLineOfService_v1_Actions actions;
};
struct DataStorageLineOfService_v1_OemActions
{};
#endif
