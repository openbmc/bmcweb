#pragma once
#include <nlohmann/json.hpp>

namespace pump
{
// clang-format off

enum class PumpType{
    Invalid,
    Liquid,
    Compressor,
};

enum class PumpMode{
    Invalid,
    Enabled,
    Disabled,
};

NLOHMANN_JSON_SERIALIZE_ENUM(PumpType, {
    {PumpType::Invalid, "Invalid"},
    {PumpType::Liquid, "Liquid"},
    {PumpType::Compressor, "Compressor"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PumpMode, {
    {PumpMode::Invalid, "Invalid"},
    {PumpMode::Enabled, "Enabled"},
    {PumpMode::Disabled, "Disabled"},
});

}
// clang-format on
