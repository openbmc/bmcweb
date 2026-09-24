// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace software_inventory
{
// clang-format off

enum class VersionScheme{
    Invalid,
    SemVer,
    DotIntegerNotation,
    OEM,
};

enum class ReleaseType{
    Invalid,
    Production,
    Prototype,
    Other,
};

enum class ImageState{
    Invalid,
    Active,
    Staged,
    Armed,
    TargetSpecific,
    OEM,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VersionScheme, {
    {VersionScheme::Invalid, "Invalid"},
    {VersionScheme::SemVer, "SemVer"},
    {VersionScheme::DotIntegerNotation, "DotIntegerNotation"},
    {VersionScheme::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ReleaseType, {
    {ReleaseType::Invalid, "Invalid"},
    {ReleaseType::Production, "Production"},
    {ReleaseType::Prototype, "Prototype"},
    {ReleaseType::Other, "Other"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ImageState, {
    {ImageState::Invalid, "Invalid"},
    {ImageState::Active, "Active"},
    {ImageState::Staged, "Staged"},
    {ImageState::Armed, "Armed"},
    {ImageState::TargetSpecific, "TargetSpecific"},
    {ImageState::OEM, "OEM"},
});

// clang-format on
} // namespace software_inventory
