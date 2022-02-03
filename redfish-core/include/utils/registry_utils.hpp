/*!
 * @file    registry_utils.cpp
 * @brief   Source code for utility functions of handling message registries.
 */

#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/beast/core/span.hpp>
#include <registries.hpp>
#include <registries/base_message_registry.hpp>
#include <registries/openbmc_message_registry.hpp>
#include <registries/resource_event_message_registry.hpp>
#include <registries/task_event_message_registry.hpp>

#include <array>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <utility>

namespace redfish
{
namespace registries
{
inline std::span<const MessageEntry>
    getRegistryFromPrefix(const std::string_view registryName)
{
    if (task_event::header.registryPrefix == registryName)
    {
        return std::span<const MessageEntry>{task_event::registry};
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return std::span<const MessageEntry>{openbmc::registry};
    }
    if (base::header.registryPrefix == registryName)
    {
        return std::span<const MessageEntry>{base::registry};
    }
    if (resource_event::header.registryPrefix == registryName)
    {
        return std::span<const MessageEntry>{resource_event::registry};
    }
    return {};
}

inline const Message*
    getMessageFromRegistry(const std::string& messageKey,
                           const std::span<const MessageEntry> registry)
{
    std::span<const MessageEntry>::iterator messageIt = std::find_if(
        registry.begin(), registry.end(),
        [&messageKey](const MessageEntry& messageEntry) {
            return std::strcmp(messageEntry.first, messageKey.c_str()) == 0;
        });
    if (messageIt != registry.end())
    {
        return &messageIt->second;
    }

    return nullptr;
}

inline std::string getPrefix(const std::string& messageID)
{
    size_t pos = messageID.find('.');
    return messageID.substr(0, pos);
}

inline const Message* getMessage(const std::string_view& messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to
    // find the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, boost::is_any_of("."));

    if (fields.size() != 4)
    {
        return nullptr;
    }
    std::string& registryName = fields[0];
    std::string& messageKey = fields[3];

    return getMessageFromRegistry(messageKey,
                                  getRegistryFromPrefix(registryName));
}

inline bool isMessageIdValid(const std::string_view messageId)
{
    const Message* msg = getMessage(messageId);
    (void)msg;
    return msg != nullptr;
}
} // namespace registries
} // namespace redfish