#ifndef THERMALMETRICS_V1
#define THERMALMETRICS_V1

#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "ThermalMetrics_v1.h"

struct ThermalMetricsV1OemActions
{};
struct ThermalMetricsV1Actions
{
    ThermalMetricsV1OemActions oem;
};
struct ThermalMetricsV1TemperatureSummary
{
    NavigationReference_ internal;
    NavigationReference_ intake;
    NavigationReference_ exhaust;
    NavigationReference_ ambient;
};
struct ThermalMetricsV1ThermalMetrics
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ThermalMetricsV1TemperatureSummary temperatureSummaryCelsius;
    NavigationReference_ temperatureReadingsCelsius;
    ThermalMetricsV1Actions actions;
};
#endif
