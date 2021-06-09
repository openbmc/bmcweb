#ifndef OUTLET_V1
#define OUTLET_V1

#include "Circuit_v1.h"
#include "NavigationReferenceRedfish.h"
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
    NavigationReferenceRedfish line1;
    NavigationReferenceRedfish line2;
    NavigationReferenceRedfish line3;
    NavigationReferenceRedfish neutral;
};
struct OutletV1Links
{
    ResourceV1Resource oem;
    NavigationReferenceRedfish branchCircuit;
};
struct OutletV1VoltageSensors
{
    NavigationReferenceRedfish line1ToLine2;
    NavigationReferenceRedfish line2ToLine3;
    NavigationReferenceRedfish line3ToLine1;
    NavigationReferenceRedfish line1ToNeutral;
    NavigationReferenceRedfish line2ToNeutral;
    NavigationReferenceRedfish line3ToNeutral;
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
    NavigationReferenceRedfish voltage;
    NavigationReferenceRedfish currentAmps;
    NavigationReferenceRedfish powerWatts;
    NavigationReferenceRedfish energykWh;
    NavigationReferenceRedfish frequencyHz;
    OutletV1VoltageSensors polyPhaseVoltage;
    OutletV1CurrentSensors polyPhaseCurrentAmps;
    OutletV1Links links;
    OutletV1Actions actions;
    bool locationIndicatorActive;
};
#endif
