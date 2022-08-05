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
namespace redfish
{

namespace messages
{

nlohmann::json getLogTaskEvent(redfish::registries::task_event::Index name,
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

inline nlohmann::json taskAborted(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskAborted, {arg1});
}

inline nlohmann::json taskCancelled(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskCanceled, {arg1});
}

inline nlohmann::json taskCompletedOK(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskCompletedOK,
                           {arg1});
}

inline nlohmann::json taskCompletedWarning(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskCompletedWarning,
                           {arg1});
}

inline nlohmann::json taskPaused(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskPaused, {arg1});
}

inline nlohmann::json taskProgressChanged(const std::string& arg1,
                                          const size_t arg2)
{
    return getLogTaskEvent(registries::task_event::Index::taskProgressChanged,
                           {arg1, std::to_string(arg2)});
}

inline nlohmann::json taskRemoved(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskRemoved, {arg1});
}

inline nlohmann::json taskResumed(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskResumed, {arg1});
}

inline nlohmann::json taskStarted(const std::string& arg1)
{
    return getLogTaskEvent(registries::task_event::Index::taskStarted, {arg1});
}

} // namespace messages

} // namespace redfish
