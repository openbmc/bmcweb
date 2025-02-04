// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <string>
#include "generated/enums/resource.hpp"

namespace redfish
{
namespace location_utils
{

/**
 * @brief Converts D-Bus interface name to ConnectorType
 *
 * @param interface D-Bus interface name
 * @return ConnectorType enum value
 */
inline resource::LocationType getLocationType(std::string_view interface)
{
    static const std::unordered_map<std::string_view, resource::LocationType>
        interfaceMap{{"xyz.openbmc_project.Inventory.Connector.Embedded",
                      resource::LocationType::Embedded},
                     {"xyz.openbmc_project.Inventory.Connector.Slot",
                      resource::LocationType::Slot}};

    auto it = interfaceMap.find(interface);
    if (it != interfaceMap.end())
    {
        return it->second;
    }
    return resource::LocationType::Invalid;
}

} // namespace location_utils
} // namespace redfish
