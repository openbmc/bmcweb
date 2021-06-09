#ifndef THERMALMETRICS_V1
#define THERMALMETRICS_V1

#include "NavigationReference.h"
#include "Resource_v1.h"
#include "ThermalMetrics_v1.h"

struct ThermalMetrics_v1_Actions
{
    ThermalMetrics_v1_OemActions oem;
};
struct ThermalMetrics_v1_OemActions
{
};
struct ThermalMetrics_v1_TemperatureSummary
{
    NavigationReference_ internal;
    NavigationReference_ intake;
    NavigationReference_ exhaust;
    NavigationReference_ ambient;
};
struct ThermalMetrics_v1_ThermalMetrics
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ThermalMetrics_v1_TemperatureSummary temperatureSummaryCelsius;
    NavigationReference_ temperatureReadingsCelsius;
    ThermalMetrics_v1_Actions actions;
};
#endif
