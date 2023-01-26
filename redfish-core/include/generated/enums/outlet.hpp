#pragma once
#include <nlohmann/json.hpp>

namespace outlet
{
// clang-format off

enum class ReceptacleType{
    Invalid,
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
    BusConnection,
};

enum class VoltageType{
    Invalid,
    AC,
    DC,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ReceptacleType, {
    {ReceptacleType::Invalid, "Invalid"},
    {ReceptacleType::NEMA_5_15R, "NEMA_5_15R"},
    {ReceptacleType::NEMA_5_20R, "NEMA_5_20R"},
    {ReceptacleType::NEMA_L5_20R, "NEMA_L5_20R"},
    {ReceptacleType::NEMA_L5_30R, "NEMA_L5_30R"},
    {ReceptacleType::NEMA_L6_20R, "NEMA_L6_20R"},
    {ReceptacleType::NEMA_L6_30R, "NEMA_L6_30R"},
    {ReceptacleType::IEC_60320_C13, "IEC_60320_C13"},
    {ReceptacleType::IEC_60320_C19, "IEC_60320_C19"},
    {ReceptacleType::CEE_7_Type_E, "CEE_7_Type_E"},
    {ReceptacleType::CEE_7_Type_F, "CEE_7_Type_F"},
    {ReceptacleType::SEV_1011_TYPE_12, "SEV_1011_TYPE_12"},
    {ReceptacleType::SEV_1011_TYPE_23, "SEV_1011_TYPE_23"},
    {ReceptacleType::BS_1363_Type_G, "BS_1363_Type_G"},
    {ReceptacleType::BusConnection, "BusConnection"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(VoltageType, {
    {VoltageType::Invalid, "Invalid"},
    {VoltageType::AC, "AC"},
    {VoltageType::DC, "DC"},
});

}
// clang-format on
