#pragma once
#include <nlohmann/json.hpp>

namespace manifest
{
// clang-format off

enum class Expand{
    Invalid,
    None,
    All,
    Relevant,
};

enum class StanzaType{
    Invalid,
    ComposeSystem,
    DecomposeSystem,
    ComposeResource,
    DecomposeResource,
    OEM,
    RegisterResourceBlock,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Expand, {
    {Expand::Invalid, "Invalid"},
    {Expand::None, "None"},
    {Expand::All, "All"},
    {Expand::Relevant, "Relevant"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(StanzaType, {
    {StanzaType::Invalid, "Invalid"},
    {StanzaType::ComposeSystem, "ComposeSystem"},
    {StanzaType::DecomposeSystem, "DecomposeSystem"},
    {StanzaType::ComposeResource, "ComposeResource"},
    {StanzaType::DecomposeResource, "DecomposeResource"},
    {StanzaType::OEM, "OEM"},
    {StanzaType::RegisterResourceBlock, "RegisterResourceBlock"},
});

}
// clang-format on
