#ifndef POWER_V1
#define POWER_V1

#include "Assembly_v1.h"
#include "NavigationReference__.h"
#include "PhysicalContext_v1.h"
#include "Power_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

enum class Power_v1_InputType {
    AC,
    DC,
};
enum class Power_v1_LineInputVoltageType {
    Unknown,
    ACLowLine,
    ACMidLine,
    ACHighLine,
    DCNeg48V,
    DC380V,
    AC120V,
    AC240V,
    AC277V,
    ACandDCWideRange,
    ACWideRange,
    DC240V,
};
enum class Power_v1_PowerLimitException {
    NoAction,
    HardPowerOff,
    LogEventOnly,
    Oem,
};
enum class Power_v1_PowerSupplyType {
    Unknown,
    AC,
    DC,
    ACorDC,
};
struct Power_v1_Actions
{
    Power_v1_OemActions oem;
};
struct Power_v1_InputRange
{
    Power_v1_InputType inputType;
    double minimumVoltage;
    double maximumVoltage;
    double minimumFrequencyHz;
    double maximumFrequencyHz;
    double outputWattage;
    Resource_v1_Resource oem;
};
struct Power_v1_OemActions
{
};
struct Power_v1_Power
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Power_v1_PowerControl powerControl;
    Power_v1_Voltage voltages;
    Power_v1_PowerSupply powerSupplies;
    Redundancy_v1_Redundancy redundancy;
    Power_v1_Actions actions;
};
struct Power_v1_PowerControl
{
    Resource_v1_Resource oem;
    std::string memberId;
    std::string name;
    double powerConsumedWatts;
    double powerRequestedWatts;
    double powerAvailableWatts;
    double powerCapacityWatts;
    double powerAllocatedWatts;
    Power_v1_PowerMetric powerMetrics;
    Power_v1_PowerLimit powerLimit;
    Resource_v1_Resource status;
    NavigationReference__ relatedItem;
    Power_v1_PowerControlActions actions;
    PhysicalContext_v1_PhysicalContext physicalContext;
};
struct Power_v1_PowerControlActions
{
    Power_v1_PowerControlOemActions oem;
};
struct Power_v1_PowerControlOemActions
{
};
struct Power_v1_PowerLimit
{
    double limitInWatts;
    Power_v1_PowerLimitException limitException;
    int64_t correctionInMs;
};
struct Power_v1_PowerMetric
{
    int64_t intervalInMin;
    double minConsumedWatts;
    double maxConsumedWatts;
    double averageConsumedWatts;
};
struct Power_v1_PowerSupply
{
    Resource_v1_Resource oem;
    std::string memberId;
    std::string name;
    Power_v1_PowerSupplyType powerSupplyType;
    Power_v1_LineInputVoltageType lineInputVoltageType;
    double lineInputVoltage;
    double powerCapacityWatts;
    double lastPowerOutputWatts;
    std::string model;
    std::string firmwareVersion;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    Resource_v1_Resource status;
    NavigationReference__ relatedItem;
    NavigationReference__ redundancy;
    std::string manufacturer;
    Power_v1_InputRange inputRanges;
    Resource_v1_Resource indicatorLED;
    Power_v1_PowerSupplyActions actions;
    Resource_v1_Resource location;
    Assembly_v1_Assembly assembly;
    double powerInputWatts;
    double powerOutputWatts;
    double efficiencyPercent;
    bool hotPluggable;
};
struct Power_v1_PowerSupplyActions
{
    Power_v1_PowerSupplyOemActions oem;
};
struct Power_v1_PowerSupplyOemActions
{
};
struct Power_v1_Voltage
{
    Resource_v1_Resource oem;
    std::string memberId;
    std::string name;
    int64_t sensorNumber;
    Resource_v1_Resource status;
    double readingVolts;
    double upperThresholdNonCritical;
    double upperThresholdCritical;
    double upperThresholdFatal;
    double lowerThresholdNonCritical;
    double lowerThresholdCritical;
    double lowerThresholdFatal;
    double minReadingRange;
    double maxReadingRange;
    PhysicalContext_v1_PhysicalContext physicalContext;
    NavigationReference__ relatedItem;
    Power_v1_VoltageActions actions;
};
struct Power_v1_VoltageActions
{
    Power_v1_VoltageOemActions oem;
};
struct Power_v1_VoltageOemActions
{
};
#endif
