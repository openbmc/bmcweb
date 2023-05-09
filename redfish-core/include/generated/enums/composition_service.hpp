#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(ComposeRequestType,

    Invalid,
    Preview,
    PreviewReserve,
    Apply,
);

NLOHMANN_JSON_SERIALIZE_ENUM(ComposeRequestFormat, {
    {ComposeRequestFormat::Invalid, "Invalid"},
    {ComposeRequestFormat::Manifest, "Manifest"},
});

BOOST_DESCRIBE_ENUM(ComposeRequestFormat,

    Invalid,
    Manifest,
);

}
// clang-format on
