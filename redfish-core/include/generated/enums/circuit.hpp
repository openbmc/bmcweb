#pragma once
#include <nlohmann/json.hpp>

namespace circuit
{
// clang-format off

enum class PowerState{
    Invalid,
    On,
    Off,
    PowerCycle,
};

enum class BreakerStates{
    Invalid,
    Normal,
    Tripped,
    Off,
};

enum class PowerRestorePolicyTypes{
    Invalid,
    AlwaysOn,
    AlwaysOff,
    LastState,
};

enum class PhaseWiringType{
    Invalid,
    OnePhase3Wire,
    TwoPhase3Wire,
    OneOrTwoPhase3Wire,
    TwoPhase4Wire,
    ThreePhase4Wire,
    ThreePhase5Wire,
};

enum class NominalVoltageType{
    Invalid,
    AC100To127V,
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
    DC16V,
    DC12V,
    DC9V,
    DC5V,
    DC3_3V,
    DC1_8V,
};

enum class PlugType{
    Invalid,
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

enum class CircuitType{
    Invalid,
    Mains,
    Branch,
    Subfeed,
    Feeder,
    Bus,
};

enum class VoltageType{
    Invalid,
    AC,
    DC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerState, {
    {PowerState::Invalid, "Invalid"},
    {PowerState::On, "On"},
    {PowerState::Off, "Off"},
    {PowerState::PowerCycle, "PowerCycle"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BreakerStates, {
    {BreakerStates::Invalid, "Invalid"},
    {BreakerStates::Normal, "Normal"},
    {BreakerStates::Tripped, "Tripped"},
    {BreakerStates::Off, "Off"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerRestorePolicyTypes, {
    {PowerRestorePolicyTypes::Invalid, "Invalid"},
    {PowerRestorePolicyTypes::AlwaysOn, "AlwaysOn"},
    {PowerRestorePolicyTypes::AlwaysOff, "AlwaysOff"},
    {PowerRestorePolicyTypes::LastState, "LastState"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PhaseWiringType, {
    {PhaseWiringType::Invalid, "Invalid"},
    {PhaseWiringType::OnePhase3Wire, "OnePhase3Wire"},
    {PhaseWiringType::TwoPhase3Wire, "TwoPhase3Wire"},
    {PhaseWiringType::OneOrTwoPhase3Wire, "OneOrTwoPhase3Wire"},
    {PhaseWiringType::TwoPhase4Wire, "TwoPhase4Wire"},
    {PhaseWiringType::ThreePhase4Wire, "ThreePhase4Wire"},
    {PhaseWiringType::ThreePhase5Wire, "ThreePhase5Wire"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NominalVoltageType, {
    {NominalVoltageType::Invalid, "Invalid"},
    {NominalVoltageType::AC100To127V, "AC100To127V"},
    {NominalVoltageType::AC100To240V, "AC100To240V"},
    {NominalVoltageType::AC100To277V, "AC100To277V"},
    {NominalVoltageType::AC120V, "AC120V"},
    {NominalVoltageType::AC200To240V, "AC200To240V"},
    {NominalVoltageType::AC200To277V, "AC200To277V"},
    {NominalVoltageType::AC208V, "AC208V"},
    {NominalVoltageType::AC230V, "AC230V"},
    {NominalVoltageType::AC240V, "AC240V"},
    {NominalVoltageType::AC240AndDC380V, "AC240AndDC380V"},
    {NominalVoltageType::AC277V, "AC277V"},
    {NominalVoltageType::AC277AndDC380V, "AC277AndDC380V"},
    {NominalVoltageType::AC400V, "AC400V"},
    {NominalVoltageType::AC480V, "AC480V"},
    {NominalVoltageType::DC48V, "DC48V"},
    {NominalVoltageType::DC240V, "DC240V"},
    {NominalVoltageType::DC380V, "DC380V"},
    {NominalVoltageType::DCNeg48V, "DCNeg48V"},
    {NominalVoltageType::DC16V, "DC16V"},
    {NominalVoltageType::DC12V, "DC12V"},
    {NominalVoltageType::DC9V, "DC9V"},
    {NominalVoltageType::DC5V, "DC5V"},
    {NominalVoltageType::DC3_3V, "DC3_3V"},
    {NominalVoltageType::DC1_8V, "DC1_8V"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PlugType, {
    {PlugType::Invalid, "Invalid"},
    {PlugType::NEMA_5_15P, "NEMA_5_15P"},
    {PlugType::NEMA_L5_15P, "NEMA_L5_15P"},
    {PlugType::NEMA_5_20P, "NEMA_5_20P"},
    {PlugType::NEMA_L5_20P, "NEMA_L5_20P"},
    {PlugType::NEMA_L5_30P, "NEMA_L5_30P"},
    {PlugType::NEMA_6_15P, "NEMA_6_15P"},
    {PlugType::NEMA_L6_15P, "NEMA_L6_15P"},
    {PlugType::NEMA_6_20P, "NEMA_6_20P"},
    {PlugType::NEMA_L6_20P, "NEMA_L6_20P"},
    {PlugType::NEMA_L6_30P, "NEMA_L6_30P"},
    {PlugType::NEMA_L14_20P, "NEMA_L14_20P"},
    {PlugType::NEMA_L14_30P, "NEMA_L14_30P"},
    {PlugType::NEMA_L15_20P, "NEMA_L15_20P"},
    {PlugType::NEMA_L15_30P, "NEMA_L15_30P"},
    {PlugType::NEMA_L21_20P, "NEMA_L21_20P"},
    {PlugType::NEMA_L21_30P, "NEMA_L21_30P"},
    {PlugType::NEMA_L22_20P, "NEMA_L22_20P"},
    {PlugType::NEMA_L22_30P, "NEMA_L22_30P"},
    {PlugType::California_CS8265, "California_CS8265"},
    {PlugType::California_CS8365, "California_CS8365"},
    {PlugType::IEC_60320_C14, "IEC_60320_C14"},
    {PlugType::IEC_60320_C20, "IEC_60320_C20"},
    {PlugType::IEC_60309_316P6, "IEC_60309_316P6"},
    {PlugType::IEC_60309_332P6, "IEC_60309_332P6"},
    {PlugType::IEC_60309_363P6, "IEC_60309_363P6"},
    {PlugType::IEC_60309_516P6, "IEC_60309_516P6"},
    {PlugType::IEC_60309_532P6, "IEC_60309_532P6"},
    {PlugType::IEC_60309_563P6, "IEC_60309_563P6"},
    {PlugType::IEC_60309_460P9, "IEC_60309_460P9"},
    {PlugType::IEC_60309_560P9, "IEC_60309_560P9"},
    {PlugType::Field_208V_3P4W_60A, "Field_208V_3P4W_60A"},
    {PlugType::Field_400V_3P5W_32A, "Field_400V_3P5W_32A"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(CircuitType, {
    {CircuitType::Invalid, "Invalid"},
    {CircuitType::Mains, "Mains"},
    {CircuitType::Branch, "Branch"},
    {CircuitType::Subfeed, "Subfeed"},
    {CircuitType::Feeder, "Feeder"},
    {CircuitType::Bus, "Bus"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(VoltageType, {
    {VoltageType::Invalid, "Invalid"},
    {VoltageType::AC, "AC"},
    {VoltageType::DC, "DC"},
});

}
// clang-format on
