#ifndef THERMAL_V1
#define THERMAL_V1

#include "Assembly_v1.h"
#include "NavigationReference__.h"
#include "PhysicalContext_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"
#include "Thermal_v1.h"

enum class Thermal_v1_ReadingUnits
{
    RPM,
    Percent,
};
struct Thermal_v1_Fan
{
    Resource_v1_Resource oem;
    std::string memberId;
    std::string fanName;
    PhysicalContext_v1_PhysicalContext physicalContext;
    Resource_v1_Resource status;
    int64_t reading;
    int64_t upperThresholdNonCritical;
    int64_t upperThresholdCritical;
    int64_t upperThresholdFatal;
    int64_t lowerThresholdNonCritical;
    int64_t lowerThresholdCritical;
    int64_t lowerThresholdFatal;
    int64_t minReadingRange;
    int64_t maxReadingRange;
    NavigationReference__ relatedItem;
    NavigationReference__ redundancy;
    Thermal_v1_ReadingUnits readingUnits;
    std::string name;
    std::string manufacturer;
    std::string model;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    Resource_v1_Resource indicatorLED;
    Thermal_v1_FanActions actions;
    bool hotPluggable;
    Resource_v1_Resource location;
    Assembly_v1_Assembly assembly;
    int64_t sensorNumber;
};
struct Thermal_v1_FanActions
{
    Thermal_v1_FanOemActions oem;
};
struct Thermal_v1_FanOemActions
{};
struct Thermal_v1_Temperature
{
    Resource_v1_Resource oem;
    std::string memberId;
    std::string name;
    int64_t sensorNumber;
    Resource_v1_Resource status;
    double readingCelsius;
    double upperThresholdNonCritical;
    double upperThresholdCritical;
    double upperThresholdFatal;
    double lowerThresholdNonCritical;
    double lowerThresholdCritical;
    double lowerThresholdFatal;
    double minReadingRangeTemp;
    double maxReadingRangeTemp;
    PhysicalContext_v1_PhysicalContext physicalContext;
    NavigationReference__ relatedItem;
    Thermal_v1_TemperatureActions actions;
    double deltaReadingCelsius;
    PhysicalContext_v1_PhysicalContext deltaPhysicalContext;
    int64_t maxAllowableOperatingValue;
    int64_t minAllowableOperatingValue;
    int64_t adjustedMaxAllowableOperatingValue;
    int64_t adjustedMinAllowableOperatingValue;
    int64_t upperThresholdUser;
    int64_t lowerThresholdUser;
};
struct Thermal_v1_TemperatureActions
{
    Thermal_v1_TemperatureOemActions oem;
};
struct Thermal_v1_TemperatureOemActions
{};
struct Thermal_v1_Thermal
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Thermal_v1_Temperature temperatures;
    Thermal_v1_Fan fans;
    Redundancy_v1_Redundancy redundancy;
    Resource_v1_Resource status;
    Thermal_v1_ThermalActions actions;
};
struct Thermal_v1_ThermalActions
{
    Thermal_v1_ThermalOemActions oem;
};
struct Thermal_v1_ThermalOemActions
{};
#endif
