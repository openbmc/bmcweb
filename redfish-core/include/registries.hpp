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

} // namespace redfish::registries
