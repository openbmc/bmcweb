#ifndef IOPERFORMANCELOSCAPABILITIES_V1
#define IOPERFORMANCELOSCAPABILITIES_V1

#include "IOPerformanceLineOfService_v1.h"
#include "IOPerformanceLoSCapabilities_v1.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"

enum class IOPerformanceLoSCapabilities_v1_IOAccessPattern
{
    ReadWrite,
    SequentialRead,
    SequentialWrite,
    RandomReadNew,
    RandomReadAgain,
};
struct IOPerformanceLoSCapabilities_v1_Actions
{
    IOPerformanceLoSCapabilities_v1_OemActions oem;
};
struct IOPerformanceLoSCapabilities_v1_IOPerformanceLoSCapabilities
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource identifier;
    bool iOLimitingIsSupported;
    std::string minSamplePeriod;
    std::string maxSamplePeriod;
    int64_t minSupportedIoOperationLatencyMicroseconds;
    IOPerformanceLoSCapabilities_v1_IOWorkload supportedIOWorkloads;
    IOPerformanceLineOfService_v1_IOPerformanceLineOfService
        supportedLinesOfService;
    IOPerformanceLoSCapabilities_v1_Actions actions;
};
struct IOPerformanceLoSCapabilities_v1_IOWorkload
{
    std::string name;
    IOPerformanceLoSCapabilities_v1_IOWorkloadComponent components;
};
struct IOPerformanceLoSCapabilities_v1_IOWorkloadComponent
{
    IOPerformanceLoSCapabilities_v1_IOAccessPattern iOAccessPattern;
    int64_t averageIOBytes;
    int64_t percentOfData;
    int64_t percentOfIOPS;
    std::string duration;
    Schedule_v1_Schedule schedule;
};
struct IOPerformanceLoSCapabilities_v1_OemActions
{};
#endif
