#pragma once
#include <boost/describe/enum.hpp>
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
    ImmersionTank,
    HeatExchanger,
    PowerStrip,
    Other,
};

enum class IndicatorLED{
    Invalid,
    Unknown,
    Lit,
    Blinking,
    Off,
};

enum class IntrusionSensor{
    Invalid,
    Normal,
    HardwareIntrusion,
    TamperingDetected,
};

enum class IntrusionSensorReArm{
    Invalid,
    Manual,
    Automatic,
};

enum class EnvironmentalClass{
    Invalid,
    A1,
    A2,
    A3,
    A4,
};

enum class ThermalDirection{
    Invalid,
    FrontToBack,
    BackToFront,
    TopExhaust,
    Sealed,
};

enum class DoorState{
    Invalid,
    Locked,
    Closed,
    LockedAndOpen,
    Open,
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
    {ChassisType::ImmersionTank, "ImmersionTank"},
    {ChassisType::HeatExchanger, "HeatExchanger"},
    {ChassisType::PowerStrip, "PowerStrip"},
    {ChassisType::Other, "Other"},
});

BOOST_DESCRIBE_ENUM(ChassisType,

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
    ImmersionTank,
    HeatExchanger,
    PowerStrip,
    Other,
);

NLOHMANN_JSON_SERIALIZE_ENUM(IndicatorLED, {
    {IndicatorLED::Invalid, "Invalid"},
    {IndicatorLED::Unknown, "Unknown"},
    {IndicatorLED::Lit, "Lit"},
    {IndicatorLED::Blinking, "Blinking"},
    {IndicatorLED::Off, "Off"},
});

BOOST_DESCRIBE_ENUM(IndicatorLED,

    Invalid,
    Unknown,
    Lit,
    Blinking,
    Off,
);

NLOHMANN_JSON_SERIALIZE_ENUM(IntrusionSensor, {
    {IntrusionSensor::Invalid, "Invalid"},
    {IntrusionSensor::Normal, "Normal"},
    {IntrusionSensor::HardwareIntrusion, "HardwareIntrusion"},
    {IntrusionSensor::TamperingDetected, "TamperingDetected"},
});

BOOST_DESCRIBE_ENUM(IntrusionSensor,

    Invalid,
    Normal,
    HardwareIntrusion,
    TamperingDetected,
);

NLOHMANN_JSON_SERIALIZE_ENUM(IntrusionSensorReArm, {
    {IntrusionSensorReArm::Invalid, "Invalid"},
    {IntrusionSensorReArm::Manual, "Manual"},
    {IntrusionSensorReArm::Automatic, "Automatic"},
});

BOOST_DESCRIBE_ENUM(IntrusionSensorReArm,

    Invalid,
    Manual,
    Automatic,
);

NLOHMANN_JSON_SERIALIZE_ENUM(EnvironmentalClass, {
    {EnvironmentalClass::Invalid, "Invalid"},
    {EnvironmentalClass::A1, "A1"},
    {EnvironmentalClass::A2, "A2"},
    {EnvironmentalClass::A3, "A3"},
    {EnvironmentalClass::A4, "A4"},
});

BOOST_DESCRIBE_ENUM(EnvironmentalClass,

    Invalid,
    A1,
    A2,
    A3,
    A4,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ThermalDirection, {
    {ThermalDirection::Invalid, "Invalid"},
    {ThermalDirection::FrontToBack, "FrontToBack"},
    {ThermalDirection::BackToFront, "BackToFront"},
    {ThermalDirection::TopExhaust, "TopExhaust"},
    {ThermalDirection::Sealed, "Sealed"},
});

BOOST_DESCRIBE_ENUM(ThermalDirection,

    Invalid,
    FrontToBack,
    BackToFront,
    TopExhaust,
    Sealed,
);

NLOHMANN_JSON_SERIALIZE_ENUM(DoorState, {
    {DoorState::Invalid, "Invalid"},
    {DoorState::Locked, "Locked"},
    {DoorState::Closed, "Closed"},
    {DoorState::LockedAndOpen, "LockedAndOpen"},
    {DoorState::Open, "Open"},
});

BOOST_DESCRIBE_ENUM(DoorState,

    Invalid,
    Locked,
    Closed,
    LockedAndOpen,
    Open,
);

}
// clang-format on
