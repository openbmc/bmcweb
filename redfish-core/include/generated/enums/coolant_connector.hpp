#pragma once
#include <nlohmann/json.hpp>

namespace coolant_connector
{
// clang-format off

enum class CoolantConnectorType{
    Invalid,
    Pair,
    Supply,
    Return,
    Inline,
    Closed,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CoolantConnectorType, {
    {CoolantConnectorType::Invalid, "Invalid"},
    {CoolantConnectorType::Pair, "Pair"},
    {CoolantConnectorType::Supply, "Supply"},
    {CoolantConnectorType::Return, "Return"},
    {CoolantConnectorType::Inline, "Inline"},
    {CoolantConnectorType::Closed, "Closed"},
});

}
// clang-format on
