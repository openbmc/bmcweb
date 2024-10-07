#pragma once

#include "registries/resource_event_message_registry.hpp"

#include <nlohmann/json.hpp>

namespace redfish
{
namespace messages
{

inline nlohmann::json getLogResourceEvent(
    registries::resource_event::Index name, std::span<std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= registries::resource_event::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(registries::resource_event::header,
                              registries::resource_event::registry, index,
                              args);
}

inline nlohmann::json resourceChanged()
{
    return getLogResourceEvent(
        registries::resource_event::Index::resourceChanged, {});
}

inline nlohmann::json resourceCreated()
{
    return getLogResourceEvent(
        registries::resource_event::Index::resourceCreated, {});
}

inline nlohmann::json resourceRemoved()
{
    return getLogResourceEvent(
        registries::resource_event::Index::resourceRemoved, {});
}

} // namespace messages
} // namespace redfish
