#ifndef CIRCUIT_V1
#define CIRCUIT_V1

#include "Circuit_v1.h"
#include "NavigationReference__.h"
#include "Resource_v1.h"
#include "Sensor_v1.h"

enum class Circuit_v1_BreakerStates
{
    Normal,
    Tripped,
    Off,
};
enum class Circuit_v1_CircuitType
{
    Mains,
    Branch,
    Subfeed,
    Feeder,
};
enum class Circuit_v1_NominalVoltageType
{
    AC100To240V,
    AC100To277V,
    AC120V,
    AC200To240V,
    AC200To277V,
    AC208V,
    AC230V,
    AC240V,
    AC240AndDC380V,
    AC277V,
    AC277AndDC380V,
    AC400V,
    AC480V,
    DC240V,
    DC380V,
    DCNeg48V,
};
enum class Circuit_v1_PhaseWiringType
{
    OnePhase3Wire,
    TwoPhase3Wire,
    OneOrTwoPhase3Wire,
    TwoPhase4Wire,
    ThreePhase4Wire,
    ThreePhase5Wire,
};
enum class Circuit_v1_PlugType
{
    NEMA_5_15P,
    NEMA_L5_15P,
    NEMA_5_20P,
    NEMA_L5_20P,
    NEMA_L5_30P,
    NEMA_6_15P,
    NEMA_L6_15P,
    NEMA_6_20P,
    NEMA_L6_20P,
    NEMA_L6_30P,
    NEMA_L14_20P,
    NEMA_L14_30P,
    NEMA_L15_20P,
    NEMA_L15_30P,
    NEMA_L21_20P,
    NEMA_L21_30P,
    NEMA_L22_20P,
    NEMA_L22_30P,
    California_CS8265,
    California_CS8365,
    IEC_60320_C14,
    IEC_60320_C20,
    IEC_60309_316P6,
    IEC_60309_332P6,
    IEC_60309_363P6,
    IEC_60309_516P6,
    IEC_60309_532P6,
    IEC_60309_563P6,
    IEC_60309_460P9,
    IEC_60309_560P9,
    Field_208V_3P4W_60A,
    Field_400V_3P5W_32A,
};
enum class Circuit_v1_PowerRestorePolicyTypes
{
    AlwaysOn,
    AlwaysOff,
    LastState,
};
enum class Circuit_v1_PowerState
{
    On,
    Off,
};
enum class Circuit_v1_VoltageType
{
    AC,
    DC,
};
struct Circuit_v1_Actions
{
    Circuit_v1_OemActions oem;
};
struct Circuit_v1_Circuit
{
    Resource_v1_Resource oem;
    std::string id;
    std::string description;
    std::string name;
    Resource_v1_Resource status;
    Circuit_v1_CircuitType circuitType;
    bool criticalCircuit;
    Sensor_v1_Sensor electricalContext;
    Circuit_v1_PhaseWiringType phaseWiringType;
    Circuit_v1_VoltageType voltageType;
    Circuit_v1_PlugType plugType;
    Circuit_v1_NominalVoltageType nominalVoltage;
    double ratedCurrentAmps;
    Resource_v1_Resource indicatorLED;
    Circuit_v1_BreakerStates breakerState;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    double powerRestoreDelaySeconds;
    Circuit_v1_PowerRestorePolicyTypes powerRestorePolicy;
    Resource_v1_Resource powerState;
    bool powerEnabled;
    NavigationReference__ voltage;
    NavigationReference__ currentAmps;
    NavigationReference__ powerWatts;
    NavigationReference__ energykWh;
    NavigationReference__ frequencyHz;
    Circuit_v1_VoltageSensors polyPhaseVoltage;
    Circuit_v1_CurrentSensors polyPhaseCurrentAmps;
    Circuit_v1_PowerSensors polyPhasePowerWatts;
    Circuit_v1_EnergySensors polyPhaseEnergykWh;
    Circuit_v1_Links links;
    Circuit_v1_Actions actions;
    bool locationIndicatorActive;
};
struct Circuit_v1_CurrentSensors
{
    NavigationReference__ line1;
    NavigationReference__ line2;
    NavigationReference__ line3;
    NavigationReference__ neutral;
};
struct Circuit_v1_EnergySensors
{
    NavigationReference__ line1ToLine2;
    NavigationReference__ line2ToLine3;
    NavigationReference__ line3ToLine1;
    NavigationReference__ line1ToNeutral;
    NavigationReference__ line2ToNeutral;
    NavigationReference__ line3ToNeutral;
};
struct Circuit_v1_Links
{
    Resource_v1_Resource oem;
    NavigationReference__ branchCircuit;
    NavigationReference__ outlets;
};
struct Circuit_v1_OemActions
{};
struct Circuit_v1_PowerSensors
{
    NavigationReference__ line1ToLine2;
    NavigationReference__ line2ToLine3;
    NavigationReference__ line3ToLine1;
    NavigationReference__ line1ToNeutral;
    NavigationReference__ line2ToNeutral;
    NavigationReference__ line3ToNeutral;
};
struct Circuit_v1_VoltageSensors
{
    NavigationReference__ line1ToLine2;
    NavigationReference__ line2ToLine3;
    NavigationReference__ line3ToLine1;
    NavigationReference__ line1ToNeutral;
    NavigationReference__ line2ToNeutral;
    NavigationReference__ line3ToNeutral;
};
#endif
