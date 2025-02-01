// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "registries.hpp"

#include "registries_selector.hpp"
#include "str_utility.hpp"

#include <algorithm>
#include <cstring>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace redfish::registries
{

const Message* getMessageFromRegistry(std::string_view messageKey,
                                      std::span<const MessageEntry> registry)
{
    std::span<const MessageEntry>::iterator messageIt = std::ranges::find_if(
        registry, [&messageKey](const MessageEntry& messageEntry) {
            return messageKey.compare(messageEntry.first) == 0;
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
    std::array<std::string_view, 4> fields;
    if (!bmcweb::splitn(fields, messageID, '.'))
    {
        return nullptr;
    }

    std::string_view registryName = fields[0];
    std::string_view messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    // Find the right registry and check it for the MessageKey
    return getMessageFromRegistry(messageKey,
                                  getRegistryFromPrefix(registryName));
}

} // namespace redfish::registries
