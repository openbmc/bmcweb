// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <array>
#include <format>
#include <optional>
#include <string>
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
    {"ClockLimit", "xyz.openbmc_project.Control.Processor"},
}};

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

// Build Redfish control name from prefix and processor name.
// e.g. ("ClockLimit", "GPU_0") -> "ClockLimit_GPU_0"
inline std::string makeControlName(std::string_view prefix,
                                   std::string_view processorName)
{
    return std::format("{}_{}", prefix, processorName);
}

} // namespace control_utils
} // namespace redfish
