#pragma once
#include "registries/heartbeat_event_message_registry.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <span>
#include <string_view>

namespace redfish::messages
{

inline nlohmann::json
    getLogHeartbeat(redfish::registries::heartbeat_event::Index name,
                    std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::heartbeat_event::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::heartbeat_event::header,
                              redfish::registries::heartbeat_event::registry,
                              index, args);
}

/**
 * @brief Formats RedfishServiceFunctional message into JSON
 * Message body: "Redfish service is functional."
 *
 * @returns Message RedfishServiceFunctional formatted to JSON */
inline nlohmann::json redfishServiceFunctional()
{
    return getLogHeartbeat(
        registries::heartbeat_event::Index::redfishServiceFunctional, {});
}

} // namespace redfish::messages
