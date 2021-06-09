#ifndef POWERSUPPLY_V1
#define POWERSUPPLY_V1

#include "Assembly_v1.h"
#include "Circuit_v1.h"
#include "NavigationReference.h"
#include "PhysicalContext_v1.h"
#include "PowerSupplyMetrics_v1.h"
#include "PowerSupply_v1.h"
#include "Resource_v1.h"

enum class PowerSupply_v1_PowerSupplyType
{
    AC,
    DC,
    ACorDC,
};
struct PowerSupply_v1_Actions
{
    PowerSupply_v1_OemActions oem;
};
struct PowerSupply_v1_EfficiencyRating
{
    double loadPercent;
    double efficiencyPercent;
};
struct PowerSupply_v1_InputRange
{
    Circuit_v1_Circuit nominalVoltageType;
    double capacityWatts;
};
struct PowerSupply_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference_ outlet;
};
struct PowerSupply_v1_OemActions
{};
struct PowerSupply_v1_OutputRail
{
    double nominalVoltage;
    PhysicalContext_v1_PhysicalContext physicalContext;
};
struct PowerSupply_v1_PowerSupply
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    PowerSupply_v1_PowerSupplyType powerSupplyType;
    Circuit_v1_Circuit inputNominalVoltageType;
    double powerCapacityWatts;
    std::string manufacturer;
    std::string model;
    std::string firmwareVersion;
    std::string serialNumber;
    std::string partNumber;
    std::string sparePartNumber;
    Resource_v1_Resource status;
    Resource_v1_Resource location;
    bool locationIndicatorActive;
    PowerSupply_v1_InputRange inputRanges;
    PowerSupply_v1_OutputRail outputRails;
    Circuit_v1_Circuit phaseWiringType;
    Circuit_v1_Circuit plugType;
    PowerSupply_v1_EfficiencyRating efficiencyRatings;
    bool hotPluggable;
    PowerSupply_v1_Links links;
    Assembly_v1_Assembly assembly;
    PowerSupplyMetrics_v1_PowerSupplyMetrics metrics;
    PowerSupply_v1_Actions actions;
};
#endif
