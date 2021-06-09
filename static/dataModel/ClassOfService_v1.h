#ifndef CLASSOFSERVICE_V1
#define CLASSOFSERVICE_V1

#include "ClassOfService_v1.h"
#include "DataSecurityLineOfService_v1.h"
#include "DataStorageLineOfService_v1.h"
#include "IOConnectivityLineOfService_v1.h"
#include "IOPerformanceLineOfService_v1.h"
#include "Resource_v1.h"

struct ClassOfService_v1_Actions
{
    ClassOfService_v1_OemActions oem;
};
struct ClassOfService_v1_ClassOfService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    std::string classOfServiceVersion;
    DataSecurityLineOfService_v1_DataSecurityLineOfService dataSecurityLinesOfService;
    DataStorageLineOfService_v1_DataStorageLineOfService dataStorageLinesOfService;
    IOConnectivityLineOfService_v1_IOConnectivityLineOfService iOConnectivityLinesOfService;
    IOPerformanceLineOfService_v1_IOPerformanceLineOfService iOPerformanceLinesOfService;
};
struct ClassOfService_v1_OemActions
{
};
#endif
