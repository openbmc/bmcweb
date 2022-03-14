#pragma once
#include <nlohmann/json.hpp>

namespace pcie_slots
{
// clang-format off

enum class SlotTypes{
    Invalid,
    FullLength,
    HalfLength,
    LowProfile,
    Mini,
    M2,
    OEM,
    OCP3Small,
    OCP3Large,
    U2,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SlotTypes, {
    {SlotTypes::Invalid, "Invalid"},
    {SlotTypes::FullLength, "FullLength"},
    {SlotTypes::HalfLength, "HalfLength"},
    {SlotTypes::LowProfile, "LowProfile"},
    {SlotTypes::Mini, "Mini"},
    {SlotTypes::M2, "M2"},
    {SlotTypes::OEM, "OEM"},
    {SlotTypes::OCP3Small, "OCP3Small"},
    {SlotTypes::OCP3Large, "OCP3Large"},
    {SlotTypes::U2, "U2"},
});

}
// clang-format on
