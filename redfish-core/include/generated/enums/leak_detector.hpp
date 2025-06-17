// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace leak_detector
{
// clang-format off

enum class LeakDetectorType{
    Invalid,
    Moisture,
    FloatSwitch,
};

enum class ReactionType{
    Invalid,
    None,
    ForceOff,
    GracefulShutdown,
};

NLOHMANN_JSON_SERIALIZE_ENUM(LeakDetectorType, {
    {LeakDetectorType::Invalid, "Invalid"},
    {LeakDetectorType::Moisture, "Moisture"},
    {LeakDetectorType::FloatSwitch, "FloatSwitch"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ReactionType, {
    {ReactionType::Invalid, "Invalid"},
    {ReactionType::None, "None"},
    {ReactionType::ForceOff, "ForceOff"},
    {ReactionType::GracefulShutdown, "GracefulShutdown"},
});

}
// clang-format on
