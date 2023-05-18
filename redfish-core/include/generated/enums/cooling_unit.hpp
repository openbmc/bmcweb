#pragma once
#include <nlohmann/json.hpp>

namespace cooling_unit
{
// clang-format off

enum class CoolingEquipmentType{
    Invalid,
    CDU,
    HeatExchanger,
    ImmersionUnit,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CoolingEquipmentType, {
    {CoolingEquipmentType::Invalid, "Invalid"},
    {CoolingEquipmentType::CDU, "CDU"},
    {CoolingEquipmentType::HeatExchanger, "HeatExchanger"},
    {CoolingEquipmentType::ImmersionUnit, "ImmersionUnit"},
});

}
// clang-format on
