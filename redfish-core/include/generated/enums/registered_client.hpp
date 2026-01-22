// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

// clang-format off

namespace registered_client
{
enum class ClientType{
    Invalid,
    Monitor,
    Configure,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ClientType, {
    {ClientType::Invalid, "Invalid"},
    {ClientType::Monitor, "Monitor"},
    {ClientType::Configure, "Configure"},
});

// clang-format on
}
