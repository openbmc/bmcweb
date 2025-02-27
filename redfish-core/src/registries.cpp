// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "registries.hpp"

// We need the registries_selector pulled into some cpp part so that the
// registration hooks run.
// NOLINTNEXTLINE(misc-include-cleaner)
#include "registries_selector.hpp"
#include "str_utility.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace redfish::registries
{

auto allRegistries() -> std::unordered_map<std::string, RegistryEntry>&
{
    static std::unordered_map<std::string, RegistryEntry> registries;
    return registries;
}

auto getRegistryFromPrefix(const std::string& registryName)
    -> std::optional<RegistryEntryRef>
{
    auto& registries = allRegistries();
    if (auto it = registries.find(registryName); it != registries.end())
    {
        return std::ref(it->second);
    }

    return std::nullopt;
}

auto getRegistryMessagesFromPrefix(const std::string& registryName)
    -> MessageEntries
{
    auto registry = getRegistryFromPrefix(registryName);
    if (!registry)
    {
        return {};
    }

    return registry->get().entries;
}

const Message* getMessageFromRegistry(const std::string& messageKey,
                                      std::span<const MessageEntry> registry)
{
    std::span<const MessageEntry>::iterator messageIt = std::ranges::find_if(
        registry, [&messageKey](const MessageEntry& messageEntry) {
            return std::strcmp(messageEntry.first, messageKey.c_str()) == 0;
        });
    if (messageIt != registry.end())
    {
        return &messageIt->second;
    }

    return nullptr;
}

const Message* getMessage(std::string_view messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    bmcweb::split(fields, messageID, '.');
    if (fields.size() != 4)
    {
        return nullptr;
    }

    const std::string& registryName = fields[0];
    const std::string& messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    return getMessageFromRegistry(messageKey,
                                  getRegistryMessagesFromPrefix(registryName));
}

} // namespace redfish::registries
