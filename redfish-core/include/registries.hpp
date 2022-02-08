/*
// Copyright (c) 2019 Intel Corporation
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
#pragma once

#include <nlohmann/json.hpp>

#include <span>
#include <string>
#include <string_view>

namespace redfish::registries
{
struct Header
{
    const char* copyright;
    const char* type;
    const char* id;
    const char* name;
    const char* language;
    const char* description;
    const char* registryPrefix;
    const char* registryVersion;
    const char* owningEntity;
};

struct Message
{
    const char* description;
    const char* message;
    const char* messageSeverity;
    const size_t numberOfArgs;
    std::array<const char*, 5> paramTypes;
    const char* resolution;
};
using MessageEntry = std::pair<const char*, const Message>;

inline void fillMessageArgs(const std::span<const std::string_view> messageArgs,
                            std::string& msg)
{
    int i = 0;
    for (const std::string_view& messageArg : messageArgs)
    {
        std::string argStr = "%" + std::to_string(i + 1);
        size_t argPos = msg.find(argStr);
        if (argPos != std::string::npos)
        {
            msg.replace(argPos, argStr.length(), messageArg);
        }
        i++;
    }
}

inline nlohmann::json getLogFromRegistry(const Header& header,
                                         std::span<const MessageEntry> registry,
                                         size_t index,
                                         std::span<const std::string_view> args)
{
    const redfish::registries::MessageEntry& entry = registry[index];
    // Intentionally make a copy of the string, so we can append in the
    // parameters.
    std::string msg = entry.second.message;
    redfish::registries::fillMessageArgs(args, msg);
    nlohmann::json jArgs = nlohmann::json::array();
    for (const std::string_view arg : args)
    {
        jArgs.push_back(arg);
    }
    std::string msgId = header.id;
    msgId += ".";
    msgId += entry.first;
    return {{"@odata.type", "#Message.v1_1_1.Message"},
            {"MessageId", std::move(msgId)},
            {"Message", std::move(msg)},
            {"MessageArgs", std::move(jArgs)},
            {"MessageSeverity", entry.second.messageSeverity},
            {"Resolution", entry.second.resolution}};
}

} // namespace redfish::registries
