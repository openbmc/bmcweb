#pragma once

#include "generated/enums/log_entry.hpp"
#include "logging.hpp"
#include "persistent_data.hpp"
#include "str_utility.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace redfish
{

inline void getRegistryAndMessageKey(const std::string& messageID,
                                     std::string& registryName,
                                     std::string& messageKey)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    bmcweb::split(fields, messageID, '.');
    if (fields.size() == 4)
    {
        registryName = fields[0];
        messageKey = fields[3];
    }
}

inline bool eventMatchesFilter(const persistent_data::UserSubscription& userSub,
                               const nlohmann::json::object_t& eventMessage,
                               std::string_view resType)
{
    // If resourceTypes list is empty, assume all
    if (!userSub.resourceTypes.empty())
    {
        // Search the resourceTypes list for the subscription.
        auto resourceTypeIndex = std::ranges::find_if(
            userSub.resourceTypes, [resType](const std::string& rtEntry) {
                return rtEntry == resType;
            });
        if (resourceTypeIndex == userSub.resourceTypes.end())
        {
            BMCWEB_LOG_DEBUG("Not subscribed to this resource");
            return false;
        }
        BMCWEB_LOG_DEBUG("ResourceType {} found in the subscribed list",
                         resType);
    }

    // If registryPrefixes list is empty, don't filter events
    // send everything.
    if (!userSub.registryPrefixes.empty())
    {
        auto eventJson = eventMessage.find("MessageId");
        if (eventJson == eventMessage.end())
        {
            return false;
        }

        const std::string* messageId =
            eventJson->second.get_ptr<const std::string*>();
        if (messageId == nullptr)
        {
            BMCWEB_LOG_ERROR("MessageId wasn't a string???");
            return false;
        }

        std::string registry;
        std::string messageKey;
        getRegistryAndMessageKey(*messageId, registry, messageKey);

        auto obj = std::ranges::find(userSub.registryPrefixes, registry);
        if (obj == userSub.registryPrefixes.end())
        {
            return false;
        }
    }

    if (!userSub.originResources.empty())
    {
        auto eventJson = eventMessage.find("OriginOfCondition");
        if (eventJson == eventMessage.end())
        {
            return false;
        }

        const std::string* originOfCondition =
            eventJson->second.get_ptr<const std::string*>();
        if (originOfCondition == nullptr)
        {
            BMCWEB_LOG_ERROR("OriginOfCondition wasn't a string???");
            return false;
        }

        auto obj =
            std::ranges::find(userSub.originResources, *originOfCondition);

        if (obj == userSub.originResources.end())
        {
            return false;
        }
    }

    // If registryMsgIds list is empty, assume all
    if (!userSub.registryMsgIds.empty())
    {
        auto eventJson = eventMessage.find("MessageId");
        if (eventJson == eventMessage.end())
        {
            BMCWEB_LOG_DEBUG("'MessageId' not present");
            return false;
        }

        const std::string* messageId =
            eventJson->second.get_ptr<const std::string*>();
        if (messageId == nullptr)
        {
            BMCWEB_LOG_ERROR("EventType wasn't a string???");
            return false;
        }

        std::string registry;
        std::string messageKey;
        getRegistryAndMessageKey(*messageId, registry, messageKey);

        BMCWEB_LOG_DEBUG("extracted registry {}", registry);
        BMCWEB_LOG_DEBUG("extracted message key {}", messageKey);

        auto obj = std::ranges::find(
            userSub.registryMsgIds, std::format("{}.{}", registry, messageKey));
        if (obj == userSub.registryMsgIds.end())
        {
            BMCWEB_LOG_DEBUG("did not find registry {} in registryMsgIds",
                             registry);
            BMCWEB_LOG_DEBUG("registryMsgIds has {} entries",
                             userSub.registryMsgIds.size());
            return false;
        }
    }

    return true;
}

} // namespace redfish
