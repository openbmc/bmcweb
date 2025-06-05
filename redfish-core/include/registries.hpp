// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2019 Intel Corporation
#pragma once

#include "bmcweb_config.h"

#include <nlohmann/json.hpp>

#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace redfish::registries
{
struct Header
{
    const char* copyright;
    const char* type;
    unsigned int versionMajor;
    unsigned int versionMinor;
    unsigned int versionPatch;
    const char* name;
    const char* language;
    const char* description;
    const char* registryPrefix;
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

struct MessageId
{
    std::string registryName;
    std::string majorVersion;
    std::string minorVersion;
    std::string messageKey;
};

using MessageEntry = std::pair<const char*, const Message>;
using MessageEntries = std::span<const MessageEntry>;

struct RegistryEntry
{
    const Header& header;
    const char* url;
    MessageEntries entries;
};
using RegistryEntryRef = std::reference_wrapper<RegistryEntry>;

auto allRegistries() -> std::map<std::string, RegistryEntry>&;

auto getRegistryFromPrefix(const std::string& registryName)
    -> std::optional<RegistryEntryRef>;

auto getRegistryMessagesFromPrefix(const std::string& registryName)
    -> MessageEntries;

template <typename T>
void registerRegistry()
{
    allRegistries().emplace(T::header.registryPrefix,
                            RegistryEntry{T::header, T::url, T::registry});
}

inline std::string fillMessageArgs(
    const std::span<const std::string_view> messageArgs, std::string_view msg)
{
    std::string ret;
    size_t reserve = msg.size();
    for (std::string_view arg : messageArgs)
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
        auto it = std::from_chars(&*msg.begin(), &*msg.end(), number);
        if (it.ec != std::errc())
        {
            ret.clear();
            return ret;
        }
        msg.remove_prefix(1);
        // Redfish message args are 1 indexed.
        number--;
        if (number >= messageArgs.size())
        {
            ret.clear();
            return ret;
        }
        ret += messageArgs[number];
    }
    ret += msg;
    return ret;
}

inline nlohmann::json::object_t getLogFromRegistry(
    const Header& header, std::span<const MessageEntry> registry, size_t index,
    std::span<const std::string_view> args)
{
    const redfish::registries::MessageEntry& entry = registry[index];
    // Intentionally make a copy of the string, so we can append in the
    // parameters.
    std::string msg =
        redfish::registries::fillMessageArgs(args, entry.second.message);
    nlohmann::json jArgs = nlohmann::json::array();
    for (std::string_view arg : args)
    {
        jArgs.push_back(arg);
    }
    std::string msgId =
        std::format("{}.{}.{}.{}", header.registryPrefix, header.versionMajor,
                    header.versionMinor, entry.first);
    nlohmann::json::object_t response;
    response["@odata.type"] = "#Message.v1_1_1.Message";
    response["MessageId"] = std::move(msgId);
    response["Message"] = std::move(msg);
    response["MessageArgs"] = std::move(jArgs);
    response["MessageSeverity"] = entry.second.messageSeverity;
    response["Resolution"] = entry.second.resolution;
    return response;
}

const Message* getMessage(std::string_view messageID);

const Message* getMessageFromRegistry(const std::string& messageKey,
                                      std::span<const MessageEntry> registry);

std::optional<MessageId> getMessageComponents(std::string_view message);

} // namespace redfish::registries
