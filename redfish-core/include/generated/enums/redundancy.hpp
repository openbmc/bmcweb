// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace redundancy
{
// clang-format off

enum class RedundancyMode{
    Invalid,
    Failover,
    Nm,
    Sharing,
    Sparing,
    NotRedundant,
};

enum class RedundancyType{
    Invalid,
    Failover,
    NPlusM,
    Sharing,
    Sparing,
    NotRedundant,
};

NLOHMANN_JSON_SERIALIZE_ENUM(RedundancyMode, {
    {RedundancyMode::Invalid, "Invalid"},
    {RedundancyMode::Failover, "Failover"},
    {RedundancyMode::Nm, "N+m"},
    {RedundancyMode::Sharing, "Sharing"},
    {RedundancyMode::Sparing, "Sparing"},
    {RedundancyMode::NotRedundant, "NotRedundant"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(RedundancyType, {
    {RedundancyType::Invalid, "Invalid"},
    {RedundancyType::Failover, "Failover"},
    {RedundancyType::NPlusM, "NPlusM"},
    {RedundancyType::Sharing, "Sharing"},
    {RedundancyType::Sparing, "Sparing"},
    {RedundancyType::NotRedundant, "NotRedundant"},
});

}
// clang-format on
