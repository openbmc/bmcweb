/*
// Copyright (c) 2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
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

inline nlohmann::json taskAborted(const std::string& arg1,
                                  const std::string& arg2,
                                  const std::string& arg3,
                                  const std::string& arg4)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskAborted"},
        {"Message", "The task with id " + arg1 + " has been aborted."},
        {"MessageArgs", {arg1, arg2, arg3, arg4}},
        {"Severity", "Critical"},
        {"Resolution", "None."},
        {"Oem",
         {{"OpenBMC",
           {{"@odata.type", "#OemMessage.v1_0_0.Message"},
            {"AbortReason", arg2},
            {"AdditionalData", arg3},
            {"EventId", arg4}}}}}};
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
