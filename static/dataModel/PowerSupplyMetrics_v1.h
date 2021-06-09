#ifndef POWERSUPPLYMETRICS_V1
#define POWERSUPPLYMETRICS_V1

#include "NavigationReference.h"
#include "PowerSupplyMetrics_v1.h"
#include "Resource_v1.h"

struct PowerSupplyMetrics_v1_Actions
{
    PowerSupplyMetrics_v1_OemActions oem;
};
struct PowerSupplyMetrics_v1_CurrentSensors
{
    NavigationReference_ input;
    NavigationReference_ inputSecondary;
    NavigationReference_ output3Volt;
    NavigationReference_ output5Volt;
    NavigationReference_ output12Volt;
    NavigationReference_ output48Volt;
    NavigationReference_ outputAux;
};
struct PowerSupplyMetrics_v1_OemActions
{};
struct PowerSupplyMetrics_v1_PowerSensors
{
    NavigationReference_ input;
    NavigationReference_ inputSecondary;
    NavigationReference_ output;
};
struct PowerSupplyMetrics_v1_PowerSupplyMetrics
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
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
    PowerSupplyMetrics_v1_Actions actions;
};
struct PowerSupplyMetrics_v1_VoltageSensors
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
