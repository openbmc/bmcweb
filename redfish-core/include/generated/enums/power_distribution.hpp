#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace power_distribution
{
// clang-format off

enum class PowerEquipmentType{
    Invalid,
    RackPDU,
    FloorPDU,
    ManualTransferSwitch,
    AutomaticTransferSwitch,
    Switchgear,
    PowerShelf,
    Bus,
    BatteryShelf,
};

enum class TransferSensitivityType{
    Invalid,
    High,
    Medium,
    Low,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerEquipmentType, {
    {PowerEquipmentType::Invalid, "Invalid"},
    {PowerEquipmentType::RackPDU, "RackPDU"},
    {PowerEquipmentType::FloorPDU, "FloorPDU"},
    {PowerEquipmentType::ManualTransferSwitch, "ManualTransferSwitch"},
    {PowerEquipmentType::AutomaticTransferSwitch, "AutomaticTransferSwitch"},
    {PowerEquipmentType::Switchgear, "Switchgear"},
    {PowerEquipmentType::PowerShelf, "PowerShelf"},
    {PowerEquipmentType::Bus, "Bus"},
    {PowerEquipmentType::BatteryShelf, "BatteryShelf"},
});

BOOST_DESCRIBE_ENUM(PowerEquipmentType,

    Invalid,
    RackPDU,
    FloorPDU,
    ManualTransferSwitch,
    AutomaticTransferSwitch,
    Switchgear,
    PowerShelf,
    Bus,
    BatteryShelf,
);

NLOHMANN_JSON_SERIALIZE_ENUM(TransferSensitivityType, {
    {TransferSensitivityType::Invalid, "Invalid"},
    {TransferSensitivityType::High, "High"},
    {TransferSensitivityType::Medium, "Medium"},
    {TransferSensitivityType::Low, "Low"},
});

BOOST_DESCRIBE_ENUM(TransferSensitivityType,

    Invalid,
    High,
    Medium,
    Low,
);

}
// clang-format on
