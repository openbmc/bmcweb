#pragma once
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/bios_message_registry.hpp"
#include "registries/heartbeat_event_message_registry.hpp"
#include "registries/license_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/resource_event_message_registry.hpp"
#include "registries/task_event_message_registry.hpp"
#include "registries/telemetry_message_registry.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace redfish::registries
{
struct HeaderAndUrl
{
    const Header& header;
    const char* url;
};

inline std::optional<registries::HeaderAndUrl>
    getRegistryHeaderAndUrlFromPrefix(std::string_view registryName)
{
    if (base::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{base::header, base::url};
    }
    if (bios::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{bios::header, bios::url};
    }
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{heartbeat_event::header, heartbeat_event::url};
    }
    if (license::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{license::header, license::url};
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{openbmc::header, openbmc::url};
    }
    if (resource_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{resource_event::header, resource_event::url};
    }
    if (task_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{task_event::header, task_event::url};
    }
    if (telemetry::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{telemetry::header, telemetry::url};
    }
    return std::nullopt;
}

inline std::span<const MessageEntry>
    getRegistryFromPrefix(std::string_view registryName)
{
    if (base::header.registryPrefix == registryName)
    {
        return {base::registry};
    }
    if (bios::header.registryPrefix == registryName)
    {
        // TODO - does this need a different registry?
        return {openbmc::registry};
    }
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return {heartbeat_event::registry};
    }
    if (license::header.registryPrefix == registryName)
    {
        return {license::registry};
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return {openbmc::registry};
    }
    if (resource_event::header.registryPrefix == registryName)
    {
        return {resource_event::registry};
    }
    if (task_event::header.registryPrefix == registryName)
    {
        return {task_event::registry};
    }
    if (telemetry::header.registryPrefix == registryName)
    {
        return {telemetry::registry};
    }
    return {openbmc::registry};
}
} // namespace redfish::registries
