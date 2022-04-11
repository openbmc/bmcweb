#include "registries.hpp"

#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <string>
#include <vector>

namespace redfish::registries
{

// for fixing potential memory leak
const static auto splitPred = boost::is_any_of(".");

const Message* getMessage(const std::string_view& messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, splitPred);
    std::string& registryName = fields[0];
    std::string& messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    if (std::string(base::header.registryPrefix) == registryName)
    {
        return getMessageFromRegistry(
            messageKey, std::span<const MessageEntry>(base::registry));
    }
    if (std::string(openbmc::header.registryPrefix) == registryName)
    {
        return getMessageFromRegistry(
            messageKey, std::span<const MessageEntry>(openbmc::registry));
    }
    return nullptr;
}

const Message*
    getMessageFromRegistry(const std::string& messageKey,
                           const std::span<const MessageEntry>& registry)
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

} // namespace redfish::registries