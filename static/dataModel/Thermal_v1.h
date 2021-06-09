#ifndef THERMAL_V1
#define THERMAL_V1

#include "Assembly_v1.h"
#include "NavigationReference_.h"
#include "PhysicalContext_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "Thermal_v1.h"

enum class ThermalV1ReadingUnits
{
    RPM,
    Percent,
};
struct ThermalV1FanOemActions
{};
struct ThermalV1FanActions
{
    ThermalV1FanOemActions oem;
};
struct ThermalV1Fan
{
    ResourceV1Resource oem;
    std::string memberId;
    std::string fanName;
    PhysicalContextV1PhysicalContext physicalContext;
    ResourceV1Resource status;
    int64_t reading;
    int64_t upperThresholdNonCritical;
    int64_t upperThresholdCritical;
    int64_t upperThresholdFatal;
    int64_t lowerThresholdNonCritical;
    int64_t lowerThresholdCritical;
    int64_t lowerThresholdFatal;
    int64_t minReadingRange;
    int64_t maxReadingRange;
    NavigationReference_ relatedItem;
    NavigationReference_ redundancy;
    ThermalV1ReadingUnits readingUnits;
    std::string name;
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    ResourceV1Resource indicatorLED;
    ThermalV1FanActions actions;
    bool hotPluggable;
    ResourceV1Resource location;
    AssemblyV1Assembly assembly;
    int64_t sensorNumber;
};
struct ThermalV1TemperatureOemActions
{};
struct ThermalV1TemperatureActions
{
    ThermalV1TemperatureOemActions oem;
};
struct ThermalV1Temperature
{
    ResourceV1Resource oem;
    std::string memberId;
    std::string name;
    int64_t sensorNumber;
    ResourceV1Resource status;
    double readingCelsius;
    double upperThresholdNonCritical;
    double upperThresholdCritical;
    double upperThresholdFatal;
    double lowerThresholdNonCritical;
    double lowerThresholdCritical;
    double lowerThresholdFatal;
    double minReadingRangeTemp;
    double maxReadingRangeTemp;
    PhysicalContextV1PhysicalContext physicalContext;
    NavigationReference_ relatedItem;
    ThermalV1TemperatureActions actions;
    double deltaReadingCelsius;
    PhysicalContextV1PhysicalContext deltaPhysicalContext;
    int64_t maxAllowableOperatingValue;
    int64_t minAllowableOperatingValue;
    int64_t adjustedMaxAllowableOperatingValue;
    int64_t adjustedMinAllowableOperatingValue;
    int64_t upperThresholdUser;
    int64_t lowerThresholdUser;
};
struct ThermalV1ThermalOemActions
{};
struct ThermalV1ThermalActions
{
    ThermalV1ThermalOemActions oem;
};
struct ThermalV1Thermal
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ThermalV1Temperature temperatures;
    ThermalV1Fan fans;
    RedundancyV1Redundancy redundancy;
    ResourceV1Resource status;
    ThermalV1ThermalActions actions;
};
#endif
