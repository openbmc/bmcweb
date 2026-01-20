// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace switch
{
// clang-format off

enum class TargetType{
    Invalid,
    FabricPort,
    HostEdgePort,
    DownstreamEdgePort,
};

NLOHMANN_JSON_SERIALIZE_ENUM(TargetType, {
    {TargetType::Invalid, "Invalid"},
    {TargetType::FabricPort, "FabricPort"},
    {TargetType::HostEdgePort, "HostEdgePort"},
    {TargetType::DownstreamEdgePort, "DownstreamEdgePort"},
});

}
// clang-format on
