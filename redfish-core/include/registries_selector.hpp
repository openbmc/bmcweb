// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/heartbeat_event_message_registry.hpp"
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
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{heartbeat_event::header, heartbeat_event::url};
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

inline std::span<const MessageEntry> getRegistryFromPrefix(
    std::string_view registryName)
{
    if (base::header.registryPrefix == registryName)
    {
        return {base::registry};
    }
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return {heartbeat_event::registry};
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
