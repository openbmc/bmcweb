#pragma once
#include <nlohmann/json.hpp>

namespace chassis
{
// clang-format off

enum class ChassisType{
    Invalid,
    Rack,
    Blade,
    Enclosure,
    StandAlone,
    RackMount,
    Card,
    Cartridge,
    Row,
    Pod,
    Expansion,
    Sidecar,
    Zone,
    Sled,
    Shelf,
    Drawer,
    Module,
    Component,
    IPBasedDrive,
    RackGroup,
    StorageEnclosure,
    Other,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ChassisType, {
    {ChassisType::Invalid, "Invalid"},
    {ChassisType::Rack, "Rack"},
    {ChassisType::Blade, "Blade"},
    {ChassisType::Enclosure, "Enclosure"},
    {ChassisType::StandAlone, "StandAlone"},
    {ChassisType::RackMount, "RackMount"},
    {ChassisType::Card, "Card"},
    {ChassisType::Cartridge, "Cartridge"},
    {ChassisType::Row, "Row"},
    {ChassisType::Pod, "Pod"},
    {ChassisType::Expansion, "Expansion"},
    {ChassisType::Sidecar, "Sidecar"},
    {ChassisType::Zone, "Zone"},
    {ChassisType::Sled, "Sled"},
    {ChassisType::Shelf, "Shelf"},
    {ChassisType::Drawer, "Drawer"},
    {ChassisType::Module, "Module"},
    {ChassisType::Component, "Component"},
    {ChassisType::IPBasedDrive, "IPBasedDrive"},
    {ChassisType::RackGroup, "RackGroup"},
    {ChassisType::StorageEnclosure, "StorageEnclosure"},
    {ChassisType::Other, "Other"},
});

enum class EnvironmentalClass{
    Invalid,
    A1,
    A2,
    A3,
    A4,
};

NLOHMANN_JSON_SERIALIZE_ENUM(EnvironmentalClass, {
    {EnvironmentalClass::Invalid, "Invalid"},
    {EnvironmentalClass::A1, "A1"},
    {EnvironmentalClass::A2, "A2"},
    {EnvironmentalClass::A3, "A3"},
    {EnvironmentalClass::A4, "A4"},
});

enum class IndicatorLED{
    Invalid,
    Unknown,
    Lit,
    Blinking,
    Off,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IndicatorLED, {
    {IndicatorLED::Invalid, "Invalid"},
    {IndicatorLED::Unknown, "Unknown"},
    {IndicatorLED::Lit, "Lit"},
    {IndicatorLED::Blinking, "Blinking"},
    {IndicatorLED::Off, "Off"},
});

enum class IntrusionSensor{
    Invalid,
    Normal,
    HardwareIntrusion,
    TamperingDetected,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IntrusionSensor, {
    {IntrusionSensor::Invalid, "Invalid"},
    {IntrusionSensor::Normal, "Normal"},
    {IntrusionSensor::HardwareIntrusion, "HardwareIntrusion"},
    {IntrusionSensor::TamperingDetected, "TamperingDetected"},
});

enum class IntrusionSensorReArm{
    Invalid,
    Manual,
    Automatic,
};

NLOHMANN_JSON_SERIALIZE_ENUM(IntrusionSensorReArm, {
    {IntrusionSensorReArm::Invalid, "Invalid"},
    {IntrusionSensorReArm::Manual, "Manual"},
    {IntrusionSensorReArm::Automatic, "Automatic"},
});

enum class PowerState{
    Invalid,
    On,
    Off,
    PoweringOn,
    PoweringOff,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PowerState, {
    {PowerState::Invalid, "Invalid"},
    {PowerState::On, "On"},
    {PowerState::Off, "Off"},
    {PowerState::PoweringOn, "PoweringOn"},
    {PowerState::PoweringOff, "PoweringOff"},
});

}
// clang-format on
