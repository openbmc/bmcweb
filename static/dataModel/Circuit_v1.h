#ifndef CIRCUIT_V1
#define CIRCUIT_V1

#include "Circuit_v1.h"
#include "NavigationReference_.h"
#include "Resource_v1.h"
#include "Sensor_v1.h"

enum class CircuitV1BreakerStates
{
    Normal,
    Tripped,
    Off,
};
enum class CircuitV1CircuitType
{
    Mains,
    Branch,
    Subfeed,
    Feeder,
};
enum class CircuitV1NominalVoltageType
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
    DC48V,
    DC240V,
    DC380V,
    DCNeg48V,
};
enum class CircuitV1PhaseWiringType
{
    OnePhase3Wire,
    TwoPhase3Wire,
    OneOrTwoPhase3Wire,
    TwoPhase4Wire,
    ThreePhase4Wire,
    ThreePhase5Wire,
};
enum class CircuitV1PlugType
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
enum class CircuitV1PowerRestorePolicyTypes
{
    AlwaysOn,
    AlwaysOff,
    LastState,
};
enum class CircuitV1PowerState
{
    On,
    Off,
};
enum class CircuitV1VoltageType
{
    AC,
    DC,
};
struct CircuitV1OemActions
{};
struct CircuitV1Actions
{
    CircuitV1OemActions oem;
};
struct CircuitV1VoltageSensors
{
    NavigationReference_ line1ToLine2;
    NavigationReference_ line2ToLine3;
    NavigationReference_ line3ToLine1;
    NavigationReference_ line1ToNeutral;
    NavigationReference_ line2ToNeutral;
    NavigationReference_ line3ToNeutral;
};
struct CircuitV1CurrentSensors
{
    NavigationReference_ line1;
    NavigationReference_ line2;
    NavigationReference_ line3;
    NavigationReference_ neutral;
};
struct CircuitV1PowerSensors
{
    NavigationReference_ line1ToLine2;
    NavigationReference_ line2ToLine3;
    NavigationReference_ line3ToLine1;
    NavigationReference_ line1ToNeutral;
    NavigationReference_ line2ToNeutral;
    NavigationReference_ line3ToNeutral;
};
struct CircuitV1EnergySensors
{
    NavigationReference_ line1ToLine2;
    NavigationReference_ line2ToLine3;
    NavigationReference_ line3ToLine1;
    NavigationReference_ line1ToNeutral;
    NavigationReference_ line2ToNeutral;
    NavigationReference_ line3ToNeutral;
};
struct CircuitV1Links
{
    ResourceV1Resource oem;
    NavigationReference_ branchCircuit;
    NavigationReference_ outlets;
};
struct CircuitV1Circuit
{
    ResourceV1Resource oem;
    std::string id;
    std::string description;
    std::string name;
    ResourceV1Resource status;
    CircuitV1CircuitType circuitType;
    bool criticalCircuit;
    SensorV1Sensor electricalContext;
    CircuitV1PhaseWiringType phaseWiringType;
    CircuitV1VoltageType voltageType;
    CircuitV1PlugType plugType;
    CircuitV1NominalVoltageType nominalVoltage;
    double ratedCurrentAmps;
    ResourceV1Resource indicatorLED;
    CircuitV1BreakerStates breakerState;
    double powerOnDelaySeconds;
    double powerOffDelaySeconds;
    double powerCycleDelaySeconds;
    double powerRestoreDelaySeconds;
    CircuitV1PowerRestorePolicyTypes powerRestorePolicy;
    ResourceV1Resource powerState;
    bool powerEnabled;
    NavigationReference_ voltage;
    NavigationReference_ currentAmps;
    NavigationReference_ powerWatts;
    NavigationReference_ energykWh;
    NavigationReference_ frequencyHz;
    CircuitV1VoltageSensors polyPhaseVoltage;
    CircuitV1CurrentSensors polyPhaseCurrentAmps;
    CircuitV1PowerSensors polyPhasePowerWatts;
    CircuitV1EnergySensors polyPhaseEnergykWh;
    CircuitV1Links links;
    CircuitV1Actions actions;
    bool locationIndicatorActive;
};
#endif
