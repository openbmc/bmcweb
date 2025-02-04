// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <string>

namespace redfish
{
namespace processor_utils
{

inline std::string toLocationType(const std::string& location)
{
    if (location ==
        "xyz.openbmc_project.Inventory.Decorator.Location.LocationTypes.Slot")
    {
        return "Slot";
    }
    if (location ==
        "xyz.openbmc_project.Inventory.Decorator.Location.LocationTypes.Embedded")
    {
        return "Embedded";
    }
    if (location ==
        "xyz.openbmc_project.Inventory.Decorator.Location.LocationTypes.Socket")
    {
        return "Socket";
    }
    if (location ==
        "xyz.openbmc_project.Inventory.Decorator.Location.LocationTypes.Backplane")
    {
        return "Backplane";
    }
    if (location ==
        "xyz.openbmc_project.Inventory.Decorator.Location.LocationTypes.Unknown")
    {
        return "Unknown";
    }
    return "";
}

} // namespace processor_utils
} // namespace redfish
