// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{
namespace control_utils
{

// Mapping from a D-Bus interface to its Redfish control type prefix.
// Note: prefix must not contain '_' (underscore is used as the separator
// between prefix and endpoint name in a Redfish ControlId).
struct ControlTypeMapping
{
    std::string_view prefix;
    std::string_view requiredInterface;
};

constexpr std::array<ControlTypeMapping, 1> controlTypeMappings = {{
    {"ClockLimit", "xyz.openbmc_project.Control.OperatingClockSpeed"},
}};

// Extract all required D-Bus interfaces from controlTypeMappings. Used when
// a caller needs to look for controls of any type (e.g. listing all controls
// under a chassis).
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

// Parsed result of a Redfish ControlId "<prefix>_<endpointName>".
struct ParsedControlId
{
    std::string prefix;
    std::string endpointName;
};

// Parse "<prefix>_<endpointName>" into components.
// Returns nullopt if the format is invalid or the prefix is not registered.
inline std::optional<ParsedControlId> parseControlId(std::string_view controlId)
{
    size_t underscore = controlId.find('_');
    if (underscore == std::string_view::npos)
    {
        return std::nullopt;
    }
    std::string_view prefixPart = controlId.substr(0, underscore);
    std::string_view suffix = controlId.substr(underscore + 1);
    if (suffix.empty())
    {
        return std::nullopt;
    }
    for (const auto& m : controlTypeMappings)
    {
        if (m.prefix == prefixPart)
        {
            return ParsedControlId{std::string(m.prefix), std::string(suffix)};
        }
    }
    return std::nullopt;
}

// Look up the control type prefix that corresponds to a D-Bus interface.
inline std::optional<std::string_view> getControlPrefixForInterface(
    std::string_view iface)
{
    for (const auto& m : controlTypeMappings)
    {
        if (m.requiredInterface == iface)
        {
            return m.prefix;
        }
    }
    return std::nullopt;
}

// Look up the D-Bus interface that corresponds to a control type prefix.
inline std::optional<std::string_view> getInterfaceForPrefix(
    std::string_view prefix)
{
    for (const auto& m : controlTypeMappings)
    {
        if (m.prefix == prefix)
        {
            return m.requiredInterface;
        }
    }
    return std::nullopt;
}

} // namespace control_utils
} // namespace redfish
