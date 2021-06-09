#ifndef SENSOR_V1
#define SENSOR_V1

#include "NavigationReferenceRedfish.h"
#include "PhysicalContext_v1.h"
#include "Resource_v1.h"
#include "Sensor_v1.h"

#include <chrono>

enum class SensorV1ElectricalContext
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
enum class SensorV1ImplementationType
{
    PhysicalSensor,
    Synthesized,
    Reported,
};
enum class SensorV1ReadingType
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
enum class SensorV1ThresholdActivation
{
    Increasing,
    Decreasing,
    Either,
};
enum class SensorV1VoltageType
{
    AC,
    DC,
};
struct SensorV1OemActions
{};
struct SensorV1Actions
{
    SensorV1OemActions oem;
};
struct SensorV1Links
{
    ResourceV1Resource oem;
};
struct SensorV1Threshold
{
    double reading;
    SensorV1ThresholdActivation activation;
    std::chrono::milliseconds dwellTime;
};
struct SensorV1Thresholds
{
    SensorV1Threshold upperCaution;
    SensorV1Threshold upperCritical;
    SensorV1Threshold upperFatal;
    SensorV1Threshold lowerCaution;
    SensorV1Threshold lowerCritical;
    SensorV1Threshold lowerFatal;
    SensorV1Threshold upperCautionUser;
    SensorV1Threshold upperCriticalUser;
    SensorV1Threshold lowerCautionUser;
    SensorV1Threshold lowerCriticalUser;
};
struct SensorV1Sensor
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    SensorV1ReadingType readingType;
    std::string dataSourceUri;
    ResourceV1Resource status;
    double reading;
    std::string readingUnits;
    PhysicalContextV1PhysicalContext physicalContext;
    PhysicalContextV1PhysicalContext physicalSubContext;
    double peakReading;
    double maxAllowableOperatingValue;
    double minAllowableOperatingValue;
    double adjustedMaxAllowableOperatingValue;
    double adjustedMinAllowableOperatingValue;
    double apparentVA;
    double reactiveVAR;
    double powerFactor;
    double loadPercent;
    ResourceV1Resource location;
    SensorV1ElectricalContext electricalContext;
    SensorV1VoltageType voltageType;
    SensorV1Thresholds thresholds;
    double readingRangeMax;
    double readingRangeMin;
    double precision;
    double accuracy;
    double sensingFrequency;
    std::chrono::time_point<std::chrono::system_clock> peakReadingTime;
    std::chrono::time_point<std::chrono::system_clock> sensorResetTime;
    SensorV1Actions actions;
    double crestFactor;
    double tHDPercent;
    double lifetimeReading;
    std::chrono::milliseconds sensingInterval;
    std::chrono::time_point<std::chrono::system_clock> readingTime;
    SensorV1ImplementationType implementation;
    NavigationReferenceRedfish relatedItem;
    double speedRPM;
    std::string deviceName;
    SensorV1Links links;
};
#endif
