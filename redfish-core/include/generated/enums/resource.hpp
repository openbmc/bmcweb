// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace resource
{
// clang-format off

enum class Id{
    Invalid,
};

enum class Description{
    Invalid,
};

enum class Name{
    Invalid,
};

enum class UUID{
    Invalid,
};

enum class State{
    Invalid,
    Enabled,
    Disabled,
    StandbyOffline,
    StandbySpare,
    InTest,
    Starting,
    Absent,
    UnavailableOffline,
    Deferring,
    Quiesced,
    Updating,
    Qualified,
    Degraded,
};

enum class Health{
    Invalid,
    OK,
    Warning,
    Critical,
};

enum class ResetType{
    Invalid,
    On,
    ForceOff,
    GracefulShutdown,
    GracefulRestart,
    ForceRestart,
    Nmi,
    ForceOn,
    PushPowerButton,
    PowerCycle,
    Suspend,
    Pause,
    Resume,
    FullPowerCycle,
};

enum class IndicatorLED{
    Invalid,
    Lit,
    Blinking,
    Off,
};

enum class PowerState{
    Invalid,
    On,
    Off,
    PoweringOn,
    PoweringOff,
    Paused,
};

enum class DurableNameFormat{
    Invalid,
    NAA,
    iQN,
    FC_WWN,
    UUID,
    EUI,
    NQN,
    NSID,
    NGUID,
    MACAddress,
    GCXLID,
};

enum class RackUnits{
    Invalid,
    OpenU,
    EIA_310,
};

enum class LocationType{
    Invalid,
    Slot,
    Bay,
    Connector,
    Socket,
    Backplane,
    Embedded,
};

enum class Reference{
    Invalid,
    Top,
    Bottom,
    Front,
    Rear,
    Left,
    Right,
    Middle,
};

enum class Orientation{
    Invalid,
    FrontToBack,
    BackToFront,
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Id, {
    {Id::Invalid, "Invalid"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Description, {
    {Description::Invalid, "Invalid"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Name, {
    {Name::Invalid, "Invalid"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(UUID, {
    {UUID::Invalid, "Invalid"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(State, {
    {State::Invalid, "Invalid"},
    {State::Enabled, "Enabled"},
    {State::Disabled, "Disabled"},
    {State::StandbyOffline, "StandbyOffline"},
    {State::StandbySpare, "StandbySpare"},
    {State::InTest, "InTest"},
    {State::Starting, "Starting"},
    {State::Absent, "Absent"},
    {State::UnavailableOffline, "UnavailableOffline"},
    {State::Deferring, "Deferring"},
    {State::Quiesced, "Quiesced"},
    {State::Updating, "Updating"},
    {State::Qualified, "Qualified"},
    {State::Degraded, "Degraded"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Health, {
    {Health::Invalid, "Invalid"},
    {Health::OK, "OK"},
    {Health::Warning, "Warning"},
    {Health::Critical, "Critical"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ResetType, {
    {ResetType::Invalid, "Invalid"},
    {ResetType::On, "On"},
    {ResetType::ForceOff, "ForceOff"},
    {ResetType::GracefulShutdown, "GracefulShutdown"},
    {ResetType::GracefulRestart, "GracefulRestart"},
    {ResetType::ForceRestart, "ForceRestart"},
    {ResetType::Nmi, "Nmi"},
    {ResetType::ForceOn, "ForceOn"},
    {ResetType::PushPowerButton, "PushPowerButton"},
    {ResetType::PowerCycle, "PowerCycle"},
    {ResetType::Suspend, "Suspend"},
    {ResetType::Pause, "Pause"},
    {ResetType::Resume, "Resume"},
    {ResetType::FullPowerCycle, "FullPowerCycle"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(IndicatorLED, {
    {IndicatorLED::Invalid, "Invalid"},
    {IndicatorLED::Lit, "Lit"},
    {IndicatorLED::Blinking, "Blinking"},
    {IndicatorLED::Off, "Off"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PowerState, {
    {PowerState::Invalid, "Invalid"},
    {PowerState::On, "On"},
    {PowerState::Off, "Off"},
    {PowerState::PoweringOn, "PoweringOn"},
    {PowerState::PoweringOff, "PoweringOff"},
    {PowerState::Paused, "Paused"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DurableNameFormat, {
    {DurableNameFormat::Invalid, "Invalid"},
    {DurableNameFormat::NAA, "NAA"},
    {DurableNameFormat::iQN, "iQN"},
    {DurableNameFormat::FC_WWN, "FC_WWN"},
    {DurableNameFormat::UUID, "UUID"},
    {DurableNameFormat::EUI, "EUI"},
    {DurableNameFormat::NQN, "NQN"},
    {DurableNameFormat::NSID, "NSID"},
    {DurableNameFormat::NGUID, "NGUID"},
    {DurableNameFormat::MACAddress, "MACAddress"},
    {DurableNameFormat::GCXLID, "GCXLID"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(RackUnits, {
    {RackUnits::Invalid, "Invalid"},
    {RackUnits::OpenU, "OpenU"},
    {RackUnits::EIA_310, "EIA_310"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(LocationType, {
    {LocationType::Invalid, "Invalid"},
    {LocationType::Slot, "Slot"},
    {LocationType::Bay, "Bay"},
    {LocationType::Connector, "Connector"},
    {LocationType::Socket, "Socket"},
    {LocationType::Backplane, "Backplane"},
    {LocationType::Embedded, "Embedded"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Reference, {
    {Reference::Invalid, "Invalid"},
    {Reference::Top, "Top"},
    {Reference::Bottom, "Bottom"},
    {Reference::Front, "Front"},
    {Reference::Rear, "Rear"},
    {Reference::Left, "Left"},
    {Reference::Right, "Right"},
    {Reference::Middle, "Middle"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Orientation, {
    {Orientation::Invalid, "Invalid"},
    {Orientation::FrontToBack, "FrontToBack"},
    {Orientation::BackToFront, "BackToFront"},
    {Orientation::TopToBottom, "TopToBottom"},
    {Orientation::BottomToTop, "BottomToTop"},
    {Orientation::LeftToRight, "LeftToRight"},
    {Orientation::RightToLeft, "RightToLeft"},
});

}
// clang-format on
