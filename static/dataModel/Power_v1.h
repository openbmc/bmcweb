#ifndef POWER_V1
#define POWER_V1

#include "Assembly_v1.h"
#include "NavigationReference_.h"
#include "PhysicalContext_v1.h"
#include "Power_v1.h"
#include "Redundancy_v1.h"
#include "Resource_v1.h"

enum class PowerV1InputType
{
    AC,
    DC,
};
enum class PowerV1LineInputVoltageType
{
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
enum class PowerV1PowerLimitException
{
    NoAction,
    HardPowerOff,
    LogEventOnly,
    Oem,
};
enum class PowerV1PowerSupplyType
{
    Unknown,
    AC,
    DC,
    ACorDC,
};
struct PowerV1OemActions
{};
struct PowerV1Actions
{
    PowerV1OemActions oem;
};
struct PowerV1InputRange
{
    PowerV1InputType inputType;
    double minimumVoltage;
    double maximumVoltage;
    double minimumFrequencyHz;
    double maximumFrequencyHz;
    double outputWattage;
    ResourceV1Resource oem;
};
struct PowerV1PowerMetric
{
    int64_t intervalInMin;
    double minConsumedWatts;
    double maxConsumedWatts;
    double averageConsumedWatts;
};
struct PowerV1PowerLimit
{
    double limitInWatts;
    PowerV1PowerLimitException limitException;
    int64_t correctionInMs;
};
struct PowerV1PowerControlOemActions
{};
struct PowerV1PowerControlActions
{
    PowerV1PowerControlOemActions oem;
};
struct PowerV1PowerControl
{
    ResourceV1Resource oem;
    std::string memberId;
    std::string name;
    double powerConsumedWatts;
    double powerRequestedWatts;
    double powerAvailableWatts;
    double powerCapacityWatts;
    double powerAllocatedWatts;
    PowerV1PowerMetric powerMetrics;
    PowerV1PowerLimit powerLimit;
    ResourceV1Resource status;
    NavigationReference_ relatedItem;
    PowerV1PowerControlActions actions;
    PhysicalContextV1PhysicalContext physicalContext;
};
struct PowerV1VoltageOemActions
{};
struct PowerV1VoltageActions
{
    PowerV1VoltageOemActions oem;
};
struct PowerV1Voltage
{
    ResourceV1Resource oem;
    std::string memberId;
    std::string name;
    int64_t sensorNumber;
    ResourceV1Resource status;
    double readingVolts;
    double upperThresholdNonCritical;
    double upperThresholdCritical;
    double upperThresholdFatal;
    double lowerThresholdNonCritical;
    double lowerThresholdCritical;
    double lowerThresholdFatal;
    double minReadingRange;
    double maxReadingRange;
    PhysicalContextV1PhysicalContext physicalContext;
    NavigationReference_ relatedItem;
    PowerV1VoltageActions actions;
};
struct PowerV1PowerSupplyOemActions
{};
struct PowerV1PowerSupplyActions
{
    PowerV1PowerSupplyOemActions oem;
};
struct PowerV1PowerSupply
{
    ResourceV1Resource oem;
    std::string memberId;
    std::string name;
    PowerV1PowerSupplyType powerSupplyType;
    PowerV1LineInputVoltageType lineInputVoltageType;
    double lineInputVoltage;
    double powerCapacityWatts;
    double lastPowerOutputWatts;
    std::string model;
    std::string firmwareVersion;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    ResourceV1Resource status;
    NavigationReference_ relatedItem;
    NavigationReference_ redundancy;
    std::string manufacturer;
    PowerV1InputRange inputRanges;
    ResourceV1Resource indicatorLED;
    PowerV1PowerSupplyActions actions;
    ResourceV1Resource location;
    AssemblyV1Assembly assembly;
    double powerInputWatts;
    double powerOutputWatts;
    double efficiencyPercent;
    bool hotPluggable;
};
struct PowerV1Power
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PowerV1PowerControl powerControl;
    PowerV1Voltage voltages;
    PowerV1PowerSupply powerSupplies;
    RedundancyV1Redundancy redundancy;
    PowerV1Actions actions;
};
#endif
