#pragma once
#include "logging.hpp"

#include <boost/beast/http/fields.hpp>
#include <boost/container/flat_map.hpp>
#include <nlohmann/json.hpp>

namespace persistent_data
{

struct UserSubscription
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
    boost::beast::http::fields httpHeaders;
    std::vector<std::string> metricReportDefinitions;

    static std::shared_ptr<UserSubscription>
        fromJson(const nlohmann::json& j, const bool loadFromOldConfig = false)
    {
        std::shared_ptr<UserSubscription> subvalue =
            std::make_shared<UserSubscription>();
        for (const auto& element : j.items())
        {
            if (element.key() == "Id")
            {
                const std::string* value =
                    element.value().get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->id = *value;
            }
            else if (element.key() == "Destination")
            {
                const std::string* value =
                    element.value().get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->destinationUrl = *value;
            }
            else if (element.key() == "Protocol")
            {
                const std::string* value =
                    element.value().get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->protocol = *value;
            }
            else if (element.key() == "DeliveryRetryPolicy")
            {
                const std::string* value =
                    element.value().get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->retryPolicy = *value;
            }
            else if (element.key() == "Context")
            {
                const std::string* value =
                    element.value().get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->customText = *value;
            }
            else if (element.key() == "EventFormatType")
            {
                const std::string* value =
                    element.value().get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->eventFormatType = *value;
            }
            else if (element.key() == "SubscriptionType")
            {
                const std::string* value =
                    element.value().get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->subscriptionType = *value;
            }
            else if (element.key() == "MessageIds")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    const std::string* value =
                        val.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->registryMsgIds.emplace_back(*value);
                }
            }
            else if (element.key() == "RegistryPrefixes")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    const std::string* value =
                        val.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->registryPrefixes.emplace_back(*value);
                }
            }
            else if (element.key() == "ResourceTypes")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    const std::string* value =
                        val.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->resourceTypes.emplace_back(*value);
                }
            }
            else if (element.key() == "HttpHeaders")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    const std::string* value =
                        val.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_ERROR << "Failed to parse value for key"
                                         << val.key();
                        continue;
                    }
                    subvalue->httpHeaders.set(val.key(), *value);
                }
            }
            else if (element.key() == "MetricReportDefinitions")
            {
                const auto& obj = element.value();
                for (const auto& val : obj.items())
                {
                    const std::string* value =
                        val.value().get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->metricReportDefinitions.emplace_back(*value);
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

        if ((subvalue->id.empty() && !loadFromOldConfig) ||
            subvalue->destinationUrl.empty() || subvalue->protocol.empty() ||
            subvalue->retryPolicy.empty() ||
            subvalue->eventFormatType.empty() ||
            subvalue->subscriptionType.empty())
        {
            BMCWEB_LOG_ERROR << "Subscription missing required field "
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
                if ((value == nullptr) ||
                    (*value > std::numeric_limits<uint32_t>::max()))
                {
                    continue;
                }
                retryAttempts = static_cast<uint32_t>(*value);
            }
            else if (element.key() == "DeliveryRetryIntervalSeconds")
            {
                const uint64_t* value =
                    element.value().get_ptr<const uint64_t*>();
                if ((value == nullptr) ||
                    (*value > std::numeric_limits<uint32_t>::max()))
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
    std::map<std::string, std::shared_ptr<UserSubscription>>
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
