#pragma once
#include <nlohmann/json.hpp>

namespace serial_interface
{
// clang-format off

enum class SignalType{
    Invalid,
    Rs232,
    Rs485,
};

enum class Parity{
    Invalid,
    None,
    Even,
    Odd,
    Mark,
    Space,
};

enum class FlowControl{
    Invalid,
    None,
    Software,
    Hardware,
};

enum class PinOut{
    Invalid,
    Cisco,
    Cyclades,
    Digi,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SignalType, {
    {SignalType::Invalid, "Invalid"},
    {SignalType::Rs232, "Rs232"},
    {SignalType::Rs485, "Rs485"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Parity, {
    {Parity::Invalid, "Invalid"},
    {Parity::None, "None"},
    {Parity::Even, "Even"},
    {Parity::Odd, "Odd"},
    {Parity::Mark, "Mark"},
    {Parity::Space, "Space"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(FlowControl, {
    {FlowControl::Invalid, "Invalid"},
    {FlowControl::None, "None"},
    {FlowControl::Software, "Software"},
    {FlowControl::Hardware, "Hardware"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PinOut, {
    {PinOut::Invalid, "Invalid"},
    {PinOut::Cisco, "Cisco"},
    {PinOut::Cyclades, "Cyclades"},
    {PinOut::Digi, "Digi"},
});

}
// clang-format on
