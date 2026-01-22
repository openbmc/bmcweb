// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace media_controller
{
enum class MediaControllerType{
    Invalid,
    Memory,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MediaControllerType, {
    {MediaControllerType::Invalid, "Invalid"},
    {MediaControllerType::Memory, "Memory"},
});

// clang-format on
}
