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

#include <array>
#include <charconv>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <utility>

// IWYU pragma: no_include <stddef.h>

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

inline std::string
    fillMessageArgs(const std::span<const std::string_view> messageArgs,
                    std::string_view msg)
{
    std::string ret;
    size_t reserve = msg.size();
    for (const std::string_view& arg : messageArgs)
    {
        reserve += arg.size();
    }
    ret.reserve(reserve);

    for (size_t stringIndex = msg.find('%'); stringIndex != std::string::npos;
         stringIndex = msg.find('%'))
    {
        ret += msg.substr(0, stringIndex);
        msg.remove_prefix(stringIndex + 1);
        size_t number = 0;
        auto it = std::from_chars(msg.data(), &*msg.end(), number);
        if (it.ec != std::errc())
        {
            return "";
        }
        msg.remove_prefix(1);
        // Redfish message args are 1 indexed.
        number--;
        if (number >= messageArgs.size())
        {
            return "";
        }
        ret += messageArgs[number];
    }
    ret += msg;
    return ret;
}

} // namespace redfish::registries
