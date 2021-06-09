#ifndef OUTLET_V1
#define OUTLET_V1

#include "Circuit_v1.h"
#include "NavigationReference_.h"
#include "Outlet_v1.h"
#include "Resource_v1.h"
#include "Sensor_v1.h"

enum class OutletV1PowerState
{
    On,
    Off,
};
enum class OutletV1ReceptacleType
{
    NEMA_5_15R,
    NEMA_5_20R,
    NEMA_L5_20R,
    NEMA_L5_30R,
    NEMA_L6_20R,
    NEMA_L6_30R,
    IEC_60320_C13,
    IEC_60320_C19,
    CEE_7_Type_E,
    CEE_7_Type_F,
    SEV_1011_TYPE_12,
    SEV_1011_TYPE_23,
    BS_1363_Type_G,
};
enum class OutletV1VoltageType
{
    AC,
    DC,
};
struct OutletV1OemActions
{};
struct OutletV1Actions
{
    OutletV1OemActions oem;
};
struct OutletV1CurrentSensors
{
    NavigationReference_ line1;
    NavigationReference_ line2;
    NavigationReference_ line3;
    NavigationReference_ neutral;
};
struct OutletV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ branchCircuit;
};
struct OutletV1VoltageSensors
{
    NavigationReference_ line1ToLine2;
    NavigationReference_ line2ToLine3;
    NavigationReference_ line3ToLine1;
    NavigationReference_ line1ToNeutral;
    NavigationReference_ line2ToNeutral;
    NavigationReference_ line3ToNeutral;
};
struct OutletV1Outlet
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    SensorV1Sensor electricalContext;
    CircuitV1Circuit phaseWiringType;
    OutletV1VoltageType voltageType;
    OutletV1ReceptacleType outletType;
    CircuitV1Circuit nominalVoltage;
    double ratedCurrentAmps;
    ResourceV1Resource indicatorLED;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    double powerRestoreDelaySeconds;
    CircuitV1Circuit powerRestorePolicy;
    ResourceV1Resource powerState;
    bool powerEnabled;
    NavigationReference_ voltage;
    NavigationReference_ currentAmps;
    NavigationReference_ powerWatts;
    NavigationReference_ energykWh;
    NavigationReference_ frequencyHz;
    OutletV1VoltageSensors polyPhaseVoltage;
    OutletV1CurrentSensors polyPhaseCurrentAmps;
    OutletV1Links links;
    OutletV1Actions actions;
    bool locationIndicatorActive;
};
#endif
