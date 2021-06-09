#ifndef IOPERFORMANCELOSCAPABILITIES_V1
#define IOPERFORMANCELOSCAPABILITIES_V1

#include "IOPerformanceLineOfService_v1.h"
#include "IOPerformanceLoSCapabilities_v1.h"
#include "Resource_v1.h"
#include "Schedule_v1.h"

enum class IOPerformanceLoSCapabilitiesV1IOAccessPattern
{
    ReadWrite,
    SequentialRead,
    SequentialWrite,
    RandomReadNew,
    RandomReadAgain,
};
struct IOPerformanceLoSCapabilitiesV1OemActions
{};
struct IOPerformanceLoSCapabilitiesV1Actions
{
    IOPerformanceLoSCapabilitiesV1OemActions oem;
};
struct IOPerformanceLoSCapabilitiesV1IOWorkloadComponent
{
    IOPerformanceLoSCapabilitiesV1IOAccessPattern iOAccessPattern;
    int64_t averageIOBytes;
    int64_t percentOfData;
    int64_t percentOfIOPS;
    std::string duration;
    ScheduleV1Schedule schedule;
};
struct IOPerformanceLoSCapabilitiesV1IOWorkload
{
    std::string name;
    IOPerformanceLoSCapabilitiesV1IOWorkloadComponent components;
};
struct IOPerformanceLoSCapabilitiesV1IOPerformanceLoSCapabilities
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource identifier;
    bool iOLimitingIsSupported;
    std::string minSamplePeriod;
    std::string maxSamplePeriod;
    int64_t minSupportedIoOperationLatencyMicroseconds;
    IOPerformanceLoSCapabilitiesV1IOWorkload supportedIOWorkloads;
    IOPerformanceLineOfServiceV1IOPerformanceLineOfService
        supportedLinesOfService;
    IOPerformanceLoSCapabilitiesV1Actions actions;
};
#endif
