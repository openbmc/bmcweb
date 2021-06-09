#ifndef IOPERFORMANCELINEOFSERVICE_V1
#define IOPERFORMANCELINEOFSERVICE_V1

#include "IOPerformanceLineOfService_v1.h"
#include "IOPerformanceLoSCapabilities_v1.h"
#include "Resource_v1.h"

struct IOPerformanceLineOfServiceV1OemActions
{};
struct IOPerformanceLineOfServiceV1Actions
{
    IOPerformanceLineOfServiceV1OemActions oem;
};
struct IOPerformanceLineOfServiceV1IOPerformanceLineOfService
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool iOOperationsPerSecondIsLimited;
    std::string samplePeriod;
    int64_t maxIOOperationsPerSecondPerTerabyte;
    int64_t averageIOOperationLatencyMicroseconds;
    IOPerformanceLoSCapabilitiesV1IOPerformanceLoSCapabilities iOWorkload;
    IOPerformanceLineOfServiceV1Actions actions;
};
#endif
