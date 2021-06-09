#ifndef POWERSUPPLYMETRICS_V1
#define POWERSUPPLYMETRICS_V1

#include "NavigationReferenceRedfish.h"
#include "PowerSupplyMetrics_v1.h"
#include "Resource_v1.h"

struct PowerSupplyMetricsV1OemActions
{};
struct PowerSupplyMetricsV1Actions
{
    PowerSupplyMetricsV1OemActions oem;
};
struct PowerSupplyMetricsV1CurrentSensors
{
    NavigationReferenceRedfish input;
    NavigationReferenceRedfish inputSecondary;
    NavigationReferenceRedfish output3Volt;
    NavigationReferenceRedfish output5Volt;
    NavigationReferenceRedfish output12Volt;
    NavigationReferenceRedfish output48Volt;
    NavigationReferenceRedfish outputAux;
};
struct PowerSupplyMetricsV1PowerSensors
{
    NavigationReferenceRedfish input;
    NavigationReferenceRedfish inputSecondary;
    NavigationReferenceRedfish output;
};
struct PowerSupplyMetricsV1PowerSupplyMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NavigationReferenceRedfish inputVoltage;
    NavigationReferenceRedfish inputCurrentAmps;
    NavigationReferenceRedfish inputPowerWatts;
    NavigationReferenceRedfish energykWh;
    NavigationReferenceRedfish frequencyHz;
    NavigationReferenceRedfish outputPowerWatts;
    NavigationReferenceRedfish railVoltage;
    NavigationReferenceRedfish railCurrentAmps;
    NavigationReferenceRedfish railPowerWatts;
    NavigationReferenceRedfish temperatureCelsius;
    NavigationReferenceRedfish fanSpeedPercent;
    PowerSupplyMetricsV1Actions actions;
};
struct PowerSupplyMetricsV1VoltageSensors
{
    NavigationReferenceRedfish input;
    NavigationReferenceRedfish inputSecondary;
    NavigationReferenceRedfish output3Volt;
    NavigationReferenceRedfish output5Volt;
    NavigationReferenceRedfish output12Volt;
    NavigationReferenceRedfish output48Volt;
    NavigationReferenceRedfish outputAux;
};
#endif
