// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/environmental_message_registry.hpp"
#include "registries/heartbeat_event_message_registry.hpp"
#include "registries/openbmc_logging_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/openbmc_state_cable_message_registry.hpp"
#include "registries/openbmc_state_leak_detector_group_message_registry.hpp"
#include "registries/resource_event_message_registry.hpp"
#include "registries/sensor_event_message_registry.hpp"
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
    if (environmental::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{environmental::header, environmental::url};
    }
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{heartbeat_event::header, heartbeat_event::url};
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{openbmc::header, openbmc::url};
    }
    if (openbmc_logging::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{openbmc_logging::header, openbmc_logging::url};
    }
    if (openbmc_state_cable::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{openbmc_state_cable::header,
                            openbmc_state_cable::url};
    }
    if (openbmc_state_leak_detector_group::header.registryPrefix ==
        registryName)
    {
        return HeaderAndUrl{openbmc_state_leak_detector_group::header,
                            openbmc_state_leak_detector_group::url};
    }
    if (resource_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{resource_event::header, resource_event::url};
    }
    if (sensor_event::header.registryPrefix == registryName)
    {
        return HeaderAndUrl{sensor_event::header, sensor_event::url};
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
    if (environmental::header.registryPrefix == registryName)
    {
        return {environmental::registry};
    }
    if (heartbeat_event::header.registryPrefix == registryName)
    {
        return {heartbeat_event::registry};
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return {openbmc::registry};
    }
    if (openbmc_logging::header.registryPrefix == registryName)
    {
        return {openbmc_logging::registry};
    }
    if (openbmc_state_cable::header.registryPrefix == registryName)
    {
        return {openbmc_state_cable::registry};
    }
    if (openbmc_state_leak_detector_group::header.registryPrefix ==
        registryName)
    {
        return {openbmc_state_leak_detector_group::registry};
    }
    if (resource_event::header.registryPrefix == registryName)
    {
        return {resource_event::registry};
    }
    if (sensor_event::header.registryPrefix == registryName)
    {
        return {sensor_event::registry};
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
