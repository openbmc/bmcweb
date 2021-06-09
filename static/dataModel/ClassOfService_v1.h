#ifndef CLASSOFSERVICE_V1
#define CLASSOFSERVICE_V1

#include "ClassOfService_v1.h"
#include "DataSecurityLineOfService_v1.h"
#include "DataStorageLineOfService_v1.h"
#include "IOConnectivityLineOfService_v1.h"
#include "IOPerformanceLineOfService_v1.h"
#include "Resource_v1.h"

struct ClassOfServiceV1OemActions
{};
struct ClassOfServiceV1Actions
{
    ClassOfServiceV1OemActions oem;
};
struct ClassOfServiceV1ClassOfService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    std::string classOfServiceVersion;
    DataSecurityLineOfServiceV1DataSecurityLineOfService
        dataSecurityLinesOfService;
    DataStorageLineOfServiceV1DataStorageLineOfService
        dataStorageLinesOfService;
    IOConnectivityLineOfServiceV1IOConnectivityLineOfService
        iOConnectivityLinesOfService;
    IOPerformanceLineOfServiceV1IOPerformanceLineOfService
        iOPerformanceLinesOfService;
};
#endif
