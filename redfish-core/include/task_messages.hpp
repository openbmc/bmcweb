// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2020 Intel Corporation
#pragma once
#include "registries/task_event_message_registry.hpp"

#include <nlohmann/json.hpp>

#include <array>
namespace redfish::messages
{

inline nlohmann::json
    getLogTaskEvent(redfish::registries::task_event::Index name,
                    std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::task_event::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::task_event::header,
                              redfish::registries::task_event::registry, index,
                              args);
}

inline nlohmann::json taskAborted(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskAborted,
                           std::to_array({arg1}));
}

inline nlohmann::json taskCancelled(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskCancelled,
                           std::to_array({arg1}));
}

inline nlohmann::json taskCompletedOK(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskCompletedOK,
                           std::to_array({arg1}));
}

inline nlohmann::json taskCompletedWarning(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskCompletedWarning,
                           std::to_array({arg1}));
}

inline nlohmann::json taskPaused(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskPaused,
                           std::to_array({arg1}));
}

inline nlohmann::json taskProgressChanged(std::string_view arg1, size_t arg2)
{
    std::string arg2Str = std::to_string(arg2);
    return getLogTaskEvent(registries::task_event::Index::taskProgressChanged,
                           std::to_array<std::string_view>({arg1, arg2Str}));
}

inline nlohmann::json taskRemoved(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskRemoved,
                           std::to_array({arg1}));
}

inline nlohmann::json taskResumed(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskResumed,
                           std::to_array({arg1}));
}

inline nlohmann::json taskStarted(std::string_view arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskStarted,
                           std::to_array({arg1}));
}

} // namespace redfish::messages
