#pragma once
#include "logging.hpp"

#include <boost/container/flat_map.hpp>
#include <nlohmann/json.hpp>

namespace persistent_data
{

struct UserSubscription : std::enable_shared_from_this<UserSubscription>
{
    std::string id;
    std::string destinationUrl;
    std::string protocol;
    std::string retryPolicy;
    std::string customText;
    std::string eventFormatType;
    std::string subscriptionType;
    std::vector<std::string> registryMsgIds;
    std::vector<std::string> registryPrefixes;
    std::vector<std::string> resourceTypes;
    std::vector<nlohmann::json> httpHeaders; // key-value pair
    std::vector<std::string> metricReportDefinitions;

    static std::shared_ptr<UserSubscription> fromJson(const nlohmann::json& j)
    {
        std::shared_ptr<UserSubscription> subvalue =
            std::make_shared<UserSubscription>();
        for (const auto& element : j.items())
        {
            const std::string* strValue =
                element.value().get_ptr<const std::string*>();
            if (element.key() == "Id")
            {
                subvalue->id = *strValue;
            }
            else if (element.key() == "Destination")
            {
                subvalue->destinationUrl = *strValue;
            }
            else if (element.key() == "Protocol")
            {
                subvalue->protocol = *strValue;
            }
            else if (element.key() == "DeliveryRetryPolicy")
            {
                subvalue->retryPolicy = *strValue;
            }
            else if (element.key() == "Context")
            {
                subvalue->customText = *strValue;
            }
            else if (element.key() == "EventFormatType")
            {
                subvalue->eventFormatType = *strValue;
            }
            else if (element.key() == "SubscriptionType")
            {
                subvalue->subscriptionType = *strValue;
            }
            else if (element.key() == "MessageIds")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    subvalue->registryMsgIds.emplace_back(val.value());
                }
            }
            else if (element.key() == "RegistryPrefixes")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    subvalue->registryPrefixes.emplace_back(val.value());
                }
            }
            else if (element.key() == "ResourceTypes")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    subvalue->resourceTypes.emplace_back(val.value());
                }
            }
            else if (element.key() == "HttpHeaders")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    subvalue->httpHeaders.emplace_back(val.value());
                }
            }
            else if (element.key() == "MetricReportDefinitions")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    subvalue->metricReportDefinitions.emplace_back(val.value());
                }
            }
            else
            {
                BMCWEB_LOG_ERROR
                    << "Got unexpected property reading persistent file: "
                    << element.key();
                continue;
            }
        }

        if (subvalue->id.empty() || subvalue->destinationUrl.empty() ||
            subvalue->protocol.empty() || subvalue->retryPolicy.empty() ||
            subvalue->customText.empty() || subvalue->eventFormatType.empty() ||
            subvalue->subscriptionType.empty() ||
            (subvalue->registryPrefixes.empty() &&
             subvalue->registryMsgIds.empty()))
        {
            BMCWEB_LOG_DEBUG << "Subscription missing required security "
                                "information, refusing to restore";
            return nullptr;
        }

        return subvalue;
    }
};

struct EventServiceConfig
{
    bool enabled = true;
    uint32_t retryAttempts = 3;
    uint32_t retryTimeoutInterval = 30;

    void fromJson(const nlohmann::json& j)
    {
        for (const auto& element : j.items())
        {
            if (element.key() == "ServiceEnabled")
            {
                const bool* value = element.value().get_ptr<const bool*>();
                if (value == nullptr)
                {
                    continue;
                }
                enabled = *value;
            }
            else if (element.key() == "DeliveryRetryAttempts")
            {
                const uint64_t* value =
                    element.value().get_ptr<const uint64_t*>();
                if (value == nullptr)
                {
                    continue;
                }
                retryAttempts = static_cast<uint32_t>(*value);
            }
            else if (element.key() == "DeliveryRetryIntervalSeconds")
            {
                const uint64_t* value =
                    element.value().get_ptr<const uint64_t*>();
                if (value == nullptr)
                {
                    continue;
                }
                retryTimeoutInterval = static_cast<uint32_t>(*value);
            }
        }
    }
};

class EventServiceStore
{
  public:
    boost::container::flat_map<std::string, std::shared_ptr<UserSubscription>>
        subscriptionsConfigMap;
    EventServiceConfig eventServiceConfig;

    static EventServiceStore& getInstance()
    {
        static EventServiceStore eventServiceStore;
        return eventServiceStore;
    }

    EventServiceConfig& getEventServiceConfig()
    {
        return eventServiceConfig;
    }
};

} // namespace persistent_data
