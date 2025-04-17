// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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

enum class BitRate{
    Invalid,
    BitRate1200,
    BitRate2400,
    BitRate4800,
    BitRate9600,
    BitRate19200,
    BitRate38400,
    BitRate57600,
    BitRate115200,
    BitRate230400,
};

enum class Parity{
    Invalid,
    None,
    Even,
    Odd,
    Mark,
    Space,
};

enum class DataBits{
    Invalid,
    DataBits5,
    DataBits6,
    DataBits7,
    DataBits8,
};

enum class StopBits{
    Invalid,
    StopBits1,
    StopBits2,
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

enum class ConnectorType{
    Invalid,
    RJ45,
    RJ11,
    DB9Female,
    DB9Male,
    DB25Female,
    DB25Male,
    USB,
    mUSB,
    uUSB,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SignalType, {
    {SignalType::Invalid, "Invalid"},
    {SignalType::Rs232, "Rs232"},
    {SignalType::Rs485, "Rs485"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(BitRate, {
    {BitRate::Invalid, "Invalid"},
    {BitRate::BitRate1200, "1200"},
    {BitRate::BitRate2400, "2400"},
    {BitRate::BitRate4800, "4800"},
    {BitRate::BitRate9600, "9600"},
    {BitRate::BitRate19200, "19200"},
    {BitRate::BitRate38400, "38400"},
    {BitRate::BitRate57600, "57600"},
    {BitRate::BitRate115200, "115200"},
    {BitRate::BitRate230400, "230400"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Parity, {
    {Parity::Invalid, "Invalid"},
    {Parity::None, "None"},
    {Parity::Even, "Even"},
    {Parity::Odd, "Odd"},
    {Parity::Mark, "Mark"},
    {Parity::Space, "Space"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(DataBits, {
    {DataBits::Invalid, "Invalid"},
    {DataBits::DataBits5, "5"},
    {DataBits::DataBits6, "6"},
    {DataBits::DataBits7, "7"},
    {DataBits::DataBits8, "8"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(StopBits, {
    {StopBits::Invalid, "Invalid"},
    {StopBits::StopBits1, "1"},
    {StopBits::StopBits2, "2"},
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

NLOHMANN_JSON_SERIALIZE_ENUM(ConnectorType, {
    {ConnectorType::Invalid, "Invalid"},
    {ConnectorType::RJ45, "RJ45"},
    {ConnectorType::RJ11, "RJ11"},
    {ConnectorType::DB9Female, "DB9 Female"},
    {ConnectorType::DB9Male, "DB9 Male"},
    {ConnectorType::DB25Female, "DB25 Female"},
    {ConnectorType::DB25Male, "DB25 Male"},
    {ConnectorType::USB, "USB"},
    {ConnectorType::mUSB, "mUSB"},
    {ConnectorType::uUSB, "uUSB"},
});

}
// clang-format on
