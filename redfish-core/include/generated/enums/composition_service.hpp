#pragma once
#include <nlohmann/json.hpp>

namespace composition_service
{
// clang-format off

enum class ComposeRequestType{
    Invalid,
    Preview,
    PreviewReserve,
    Apply,
};

enum class ComposeRequestFormat{
    Invalid,
    Manifest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ComposeRequestType, {
    {ComposeRequestType::Invalid, "Invalid"},
    {ComposeRequestType::Preview, "Preview"},
    {ComposeRequestType::PreviewReserve, "PreviewReserve"},
    {ComposeRequestType::Apply, "Apply"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ComposeRequestFormat, {
    {ComposeRequestFormat::Invalid, "Invalid"},
    {ComposeRequestFormat::Manifest, "Manifest"},
});

}
// clang-format on
