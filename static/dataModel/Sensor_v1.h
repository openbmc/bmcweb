#ifndef SENSOR_V1
#define SENSOR_V1

#include "PhysicalContext_v1.h"
#include "Resource_v1.h"
#include "Sensor_v1.h"

#include <chrono>

enum class Sensor_v1_ElectricalContext
{
    Line1,
    Line2,
    Line3,
    Neutral,
    LineToLine,
    Line1ToLine2,
    Line2ToLine3,
    Line3ToLine1,
    LineToNeutral,
    Line1ToNeutral,
    Line2ToNeutral,
    Line3ToNeutral,
    Line1ToNeutralAndL1L2,
    Line2ToNeutralAndL1L2,
    Line2ToNeutralAndL2L3,
    Line3ToNeutralAndL3L1,
    Total,
};
enum class Sensor_v1_ImplementationType
{
    PhysicalSensor,
    Synthesized,
    Reported,
};
enum class Sensor_v1_ReadingType
{
    Temperature,
    Humidity,
    Power,
    EnergykWh,
    EnergyJoules,
    Voltage,
    Current,
    Frequency,
    Pressure,
    LiquidLevel,
    Rotational,
    AirFlow,
    LiquidFlow,
    Barometric,
    Altitude,
    Percent,
};
enum class Sensor_v1_ThresholdActivation
{
    Increasing,
    Decreasing,
    Either,
};
enum class Sensor_v1_VoltageType
{
    AC,
    DC,
};
struct Sensor_v1_Actions
{
    Sensor_v1_OemActions oem;
};
struct Sensor_v1_OemActions
{};
struct Sensor_v1_Sensor
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Sensor_v1_ReadingType readingType;
    std::string dataSourceUri;
    Resource_v1_Resource status;
    double reading;
    std::string readingUnits;
    PhysicalContext_v1_PhysicalContext physicalContext;
    PhysicalContext_v1_PhysicalContext physicalSubContext;
    double peakReading;
    double maxAllowableOperatingValue;
    double minAllowableOperatingValue;
    double adjustedMaxAllowableOperatingValue;
    double adjustedMinAllowableOperatingValue;
    double apparentVA;
    double reactiveVAR;
    double powerFactor;
    double loadPercent;
    Resource_v1_Resource location;
    Sensor_v1_ElectricalContext electricalContext;
    Sensor_v1_VoltageType voltageType;
    Sensor_v1_Thresholds thresholds;
    double readingRangeMax;
    double readingRangeMin;
    double precision;
    double accuracy;
    double sensingFrequency;
    std::chrono::time_point peakReadingTime;
    std::chrono::time_point sensorResetTime;
    Sensor_v1_Actions actions;
    double crestFactor;
    double tHDPercent;
    double lifetimeReading;
    std::chrono::milliseconds sensingInterval;
    std::chrono::time_point readingTime;
    Sensor_v1_ImplementationType implementation;
};
struct Sensor_v1_Threshold
{
    double reading;
    Sensor_v1_ThresholdActivation activation;
    std::chrono::milliseconds dwellTime;
};
struct Sensor_v1_Thresholds
{
    Sensor_v1_Threshold upperCaution;
    Sensor_v1_Threshold upperCritical;
    Sensor_v1_Threshold upperFatal;
    Sensor_v1_Threshold lowerCaution;
    Sensor_v1_Threshold lowerCritical;
    Sensor_v1_Threshold lowerFatal;
};
#endif
