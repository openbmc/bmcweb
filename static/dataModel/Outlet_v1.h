#ifndef OUTLET_V1
#define OUTLET_V1

#include "Circuit_v1.h"
#include "NavigationReference__.h"
#include "Outlet_v1.h"
#include "Resource_v1.h"
#include "Sensor_v1.h"

enum class Outlet_v1_PowerState {
    On,
    Off,
};
enum class Outlet_v1_ReceptacleType {
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
enum class Outlet_v1_VoltageType {
    AC,
    DC,
};
struct Outlet_v1_Actions
{
    Outlet_v1_OemActions oem;
};
struct Outlet_v1_CurrentSensors
{
    NavigationReference__ line1;
    NavigationReference__ line2;
    NavigationReference__ line3;
    NavigationReference__ neutral;
};
struct Outlet_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ branchCircuit;
};
struct Outlet_v1_OemActions
{
};
struct Outlet_v1_Outlet
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    Sensor_v1_Sensor electricalContext;
    Circuit_v1_Circuit phaseWiringType;
    Outlet_v1_VoltageType voltageType;
    Outlet_v1_ReceptacleType outletType;
    Circuit_v1_Circuit nominalVoltage;
    double ratedCurrentAmps;
    Resource_v1_Resource indicatorLED;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    double powerRestoreDelaySeconds;
    Circuit_v1_Circuit powerRestorePolicy;
    Resource_v1_Resource powerState;
    bool powerEnabled;
    NavigationReference__ voltage;
    NavigationReference__ currentAmps;
    NavigationReference__ powerWatts;
    NavigationReference__ energykWh;
    NavigationReference__ frequencyHz;
    Outlet_v1_VoltageSensors polyPhaseVoltage;
    Outlet_v1_CurrentSensors polyPhaseCurrentAmps;
    Outlet_v1_Links links;
    Outlet_v1_Actions actions;
    bool locationIndicatorActive;
};
struct Outlet_v1_VoltageSensors
{
    NavigationReference__ line1ToLine2;
    NavigationReference__ line2ToLine3;
    NavigationReference__ line3ToLine1;
    NavigationReference__ line1ToNeutral;
    NavigationReference__ line2ToNeutral;
    NavigationReference__ line3ToNeutral;
};
#endif
