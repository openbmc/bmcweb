// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace redfish
{
namespace control_utils
{

// Mapping from a D-Bus interface to its Redfish control type prefix.
struct ControlTypeMapping
{
    std::string_view prefix;
    std::string_view requiredInterface;
};

constexpr std::array<ControlTypeMapping, 1> controlTypeMappings = {{
    {"ClockLimit", "xyz.openbmc_project.Control.OperatingClockSpeed"},
}};

// Extract all required D-Bus interfaces from controlTypeMappings.
template <size_t N>
constexpr std::array<std::string_view, N> extractRequiredInterfaces(
    const std::array<ControlTypeMapping, N>& mappings)
{
    std::array<std::string_view, N> result{};
    for (size_t i = 0; i < N; ++i)
    {
        result[i] = mappings[i].requiredInterface;
    }
    return result;
}

constexpr std::array<std::string_view, 1> controlRequiredInterfaces =
    extractRequiredInterfaces(controlTypeMappings);

// Look up which control type prefix corresponds to a D-Bus interface.
inline std::optional<std::string_view> getControlPrefixForInterface(
    std::string_view iface)
{
    for (const auto& mapping : controlTypeMappings)
    {
        if (mapping.requiredInterface == iface)
        {
            return mapping.prefix;
        }
    }
    return std::nullopt;
}

} // namespace control_utils
} // namespace redfish
