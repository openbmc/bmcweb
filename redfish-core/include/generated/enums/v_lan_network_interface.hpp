// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace v_lan_network_interface
{
// clang-format off

enum class VLANId{
    Invalid,
};

enum class VLANPriority{
    Invalid,
};

NLOHMANN_JSON_SERIALIZE_ENUM(VLANId, {
    {VLANId::Invalid, "Invalid"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(VLANPriority, {
    {VLANPriority::Invalid, "Invalid"},
});

}
// clang-format on
