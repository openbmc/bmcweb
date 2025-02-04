// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <string>

namespace redfish
{
namespace location_utils
{

enum class ConnectorType
{
    Embedded,
    Port,
    Slot,
    Invalid
};

/**
 * @brief Converts D-Bus interface name to ConnectorType
 *
 * @param interface D-Bus interface name
 * @return ConnectorType enum value
 */
inline ConnectorType getConnectorType(std::string_view interface)
{
    static const std::unordered_map<std::string_view, ConnectorType>
        interfaceMap{{"xyz.openbmc_project.Inventory.Connector.Embedded",
                      ConnectorType::Embedded},
                     {"xyz.openbmc_project.Inventory.Connector.Port",
                      ConnectorType::Port},
                     {"xyz.openbmc_project.Inventory.Connector.Slot",
                      ConnectorType::Slot}};

    auto it = interfaceMap.find(interface);
    if (it != interfaceMap.end())
    {
        return it->second;
    }
    return ConnectorType::Invalid;
}

/**
 * @brief Converts ConnectorType to string representation
 *
 * @param type ConnectorType enum value
 * @return String representation of the connector type
 */
inline std::string_view toString(ConnectorType type)
{
    switch (type)
    {
        case ConnectorType::Embedded:
            return "Embedded";
        case ConnectorType::Port:
            return "Port";
        case ConnectorType::Slot:
            return "Slot";
        default:
            return "";
    }
}

} // namespace location_utils
} // namespace redfish
