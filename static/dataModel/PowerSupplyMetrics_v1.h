#ifndef POWERSUPPLYMETRICS_V1
#define POWERSUPPLYMETRICS_V1

#include "NavigationReference_.h"
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
    NavigationReference_ input;
    NavigationReference_ inputSecondary;
    NavigationReference_ output3Volt;
    NavigationReference_ output5Volt;
    NavigationReference_ output12Volt;
    NavigationReference_ output48Volt;
    NavigationReference_ outputAux;
};
struct PowerSupplyMetricsV1PowerSensors
{
    NavigationReference_ input;
    NavigationReference_ inputSecondary;
    NavigationReference_ output;
};
struct PowerSupplyMetricsV1PowerSupplyMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    NavigationReference_ inputVoltage;
    NavigationReference_ inputCurrentAmps;
    NavigationReference_ inputPowerWatts;
    NavigationReference_ energykWh;
    NavigationReference_ frequencyHz;
    NavigationReference_ outputPowerWatts;
    NavigationReference_ railVoltage;
    NavigationReference_ railCurrentAmps;
    NavigationReference_ railPowerWatts;
    NavigationReference_ temperatureCelsius;
    NavigationReference_ fanSpeedPercent;
    PowerSupplyMetricsV1Actions actions;
};
struct PowerSupplyMetricsV1VoltageSensors
{
    NavigationReference_ input;
    NavigationReference_ inputSecondary;
    NavigationReference_ output3Volt;
    NavigationReference_ output5Volt;
    NavigationReference_ output12Volt;
    NavigationReference_ output48Volt;
    NavigationReference_ outputAux;
};
#endif
