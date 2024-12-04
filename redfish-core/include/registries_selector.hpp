#pragma once
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/heartbeat_event_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/task_event_message_registry.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace redfish::registries
{
struct HeaderAndUrl
{
    const Header* header;
    const char* url;
};

inline std::optional<registries::HeaderAndUrl>
    getRegistryHeaderAndUrlFromPrefix(std::string_view registryName)
{
    if (task_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{&task_event::header, task_event::url};
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{&openbmc::header, openbmc::url};
    }
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{&heartbeat_event::header, heartbeat_event::url};
    }
    if (base::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{&base::header, base::url};
    }
    return std::nullopt;
}

inline std::span<const MessageEntry>
    getRegistryFromPrefix(std::string_view registryName)
{
    if (task_event::header.registryPrefix == registryName)
    {
        return {task_event::registry};
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return {openbmc::registry};
    }
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return {heartbeat_event::registry};
    }
    if (base::header.registryPrefix == registryName)
    {
        return {base::registry};
    }
    return {openbmc::registry};
}
} // namespace redfish::registries
