#ifndef POWERSUPPLY_V1
#define POWERSUPPLY_V1

#include "Assembly_v1.h"
#include "Circuit_v1.h"
#include "NavigationReference_.h"
#include "PhysicalContext_v1.h"
#include "PowerSupplyMetrics_v1.h"
#include "PowerSupply_v1.h"
#include "Resource_v1.h"

#include <chrono>

enum class PowerSupplyV1PowerSupplyType
{
    AC,
    DC,
    ACorDC,
};
struct PowerSupplyV1OemActions
{};
struct PowerSupplyV1Actions
{
    PowerSupplyV1OemActions oem;
};
struct PowerSupplyV1EfficiencyRating
{
    double loadPercent;
    double efficiencyPercent;
};
struct PowerSupplyV1InputRange
{
    CircuitV1Circuit nominalVoltageType;
    double capacityWatts;
};
struct PowerSupplyV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ outlet;
};
struct PowerSupplyV1OutputRail
{
    double nominalVoltage;
    PhysicalContextV1PhysicalContext physicalContext;
};
struct PowerSupplyV1PowerSupply
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PowerSupplyV1PowerSupplyType powerSupplyType;
    CircuitV1Circuit inputNominalVoltageType;
    double powerCapacityWatts;
    std::string manufacturer;
    std::string model;
    std::string firmwareVersion;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    ResourceV1Resource status;
    ResourceV1Resource location;
    bool locationIndicatorActive;
    PowerSupplyV1InputRange inputRanges;
    PowerSupplyV1OutputRail outputRails;
    CircuitV1Circuit phaseWiringType;
    CircuitV1Circuit plugType;
    PowerSupplyV1EfficiencyRating efficiencyRatings;
    bool hotPluggable;
    PowerSupplyV1Links links;
    AssemblyV1Assembly assembly;
    PowerSupplyMetricsV1PowerSupplyMetrics metrics;
    PowerSupplyV1Actions actions;
    std::string version;
    std::chrono::time_point<std::chrono::system_clock> productionDate;
};
#endif
