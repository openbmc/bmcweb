#ifndef IOPERFORMANCELINEOFSERVICE_V1
#define IOPERFORMANCELINEOFSERVICE_V1

#include "IOPerformanceLineOfService_v1.h"
#include "IOPerformanceLoSCapabilities_v1.h"
#include "Resource_v1.h"

struct IOPerformanceLineOfService_v1_Actions
{
    IOPerformanceLineOfService_v1_OemActions oem;
};
struct IOPerformanceLineOfService_v1_IOPerformanceLineOfService
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    bool iOOperationsPerSecondIsLimited;
    std::string samplePeriod;
    int64_t maxIOOperationsPerSecondPerTerabyte;
    int64_t averageIOOperationLatencyMicroseconds;
    IOPerformanceLoSCapabilities_v1_IOPerformanceLoSCapabilities iOWorkload;
    IOPerformanceLineOfService_v1_Actions actions;
};
struct IOPerformanceLineOfService_v1_OemActions
{};
#endif
