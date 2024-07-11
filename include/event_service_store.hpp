#pragma once
#include "logging.hpp"

#include <boost/beast/http/fields.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

namespace persistent_data
{

struct UserSubscription
{
    std::string id;
    boost::urls::url destinationUrl;
    std::string protocol;
    bool verifyCertificate = true;
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
        fromJson(const nlohmann::json::object_t& j,
                 const bool loadFromOldConfig = false)
    {
        std::shared_ptr<UserSubscription> subvalue =
            std::make_shared<UserSubscription>();
        for (const auto& element : j)
        {
            if (element.first == "Id")
            {
                const std::string* value =
                    element.second.get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->id = *value;
            }
            else if (element.first == "Destination")
            {
                const std::string* value =
                    element.second.get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                boost::system::result<boost::urls::url> url =
                    boost::urls::parse_absolute_uri(*value);
                if (!url)
                {
                    continue;
                }
                subvalue->destinationUrl = std::move(*url);
            }
            else if (element.first == "Protocol")
            {
                const std::string* value =
                    element.second.get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->protocol = *value;
            }
            else if (element.first == "VerifyCertificate")
            {
                const bool* value = element.second.get_ptr<const bool*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->verifyCertificate = *value;
            }
            else if (element.first == "DeliveryRetryPolicy")
            {
                const std::string* value =
                    element.second.get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->retryPolicy = *value;
            }
            else if (element.first == "Context")
            {
                const std::string* value =
                    element.second.get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->customText = *value;
            }
            else if (element.first == "EventFormatType")
            {
                const std::string* value =
                    element.second.get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->eventFormatType = *value;
            }
            else if (element.first == "SubscriptionType")
            {
                const std::string* value =
                    element.second.get_ptr<const std::string*>();
                if (value == nullptr)
                {
                    continue;
                }
                subvalue->subscriptionType = *value;
            }
            else if (element.first == "MessageIds")
            {
                const nlohmann::json::array_t* obj =
                    element.second.get_ptr<const nlohmann::json::array_t*>();
                if (obj == nullptr)
                {
                    continue;
                }
                for (const auto& val : *obj)
                {
                    const std::string* value =
                        val.get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->registryMsgIds.emplace_back(*value);
                }
            }
            else if (element.first == "RegistryPrefixes")
            {
                const nlohmann::json::array_t* obj =
                    element.second.get_ptr<const nlohmann::json::array_t*>();
                if (obj == nullptr)
                {
                    continue;
                }
                for (const auto& val : *obj)
                {
                    const std::string* value =
                        val.get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->registryPrefixes.emplace_back(*value);
                }
            }
            else if (element.first == "ResourceTypes")
            {
                const nlohmann::json::array_t* obj =
                    element.second.get_ptr<const nlohmann::json::array_t*>();
                if (obj == nullptr)
                {
                    continue;
                }
                for (const auto& val : *obj)
                {
                    const std::string* value =
                        val.get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->resourceTypes.emplace_back(*value);
                }
            }
            else if (element.first == "HttpHeaders")
            {
                const nlohmann::json::object_t* obj =
                    element.second.get_ptr<const nlohmann::json::object_t*>();
                if (obj == nullptr)
                {
                    continue;
                }
                for (const auto& val : *obj)
                {
                    const std::string* value =
                        val.second.get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        BMCWEB_LOG_ERROR("Failed to parse value for key{}",
                                         val.first);
                        continue;
                    }
                    subvalue->httpHeaders.set(val.first, *value);
                }
            }
            else if (element.first == "MetricReportDefinitions")
            {
                const nlohmann::json::array_t* obj =
                    element.second.get_ptr<const nlohmann::json::array_t*>();
                if (obj == nullptr)
                {
                    continue;
                }
                for (const auto& val : *obj)
                {
                    const std::string* value =
                        val.get_ptr<const std::string*>();
                    if (value == nullptr)
                    {
                        continue;
                    }
                    subvalue->metricReportDefinitions.emplace_back(*value);
                }
            }
            else
            {
                BMCWEB_LOG_ERROR(
                    "Got unexpected property reading persistent file: {}",
                    element.first);
                continue;
            }
        }

        if ((subvalue->id.empty() && !loadFromOldConfig) ||
            subvalue->destinationUrl.empty() || subvalue->protocol.empty() ||
            subvalue->retryPolicy.empty() ||
            subvalue->eventFormatType.empty() ||
            subvalue->subscriptionType.empty())
        {
            BMCWEB_LOG_ERROR("Subscription missing required field "
                             "information, refusing to restore");
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

    void fromJson(const nlohmann::json::object_t& j)
    {
        for (const auto& element : j)
        {
            if (element.first == "ServiceEnabled")
            {
                const bool* value = element.second.get_ptr<const bool*>();
                if (value == nullptr)
                {
                    continue;
                }
                enabled = *value;
            }
            else if (element.first == "DeliveryRetryAttempts")
            {
                const uint64_t* value =
                    element.second.get_ptr<const uint64_t*>();
                if ((value == nullptr) ||
                    (*value > std::numeric_limits<uint32_t>::max()))
                {
                    continue;
                }
                retryAttempts = static_cast<uint32_t>(*value);
            }
            else if (element.first == "DeliveryRetryIntervalSeconds")
            {
                const uint64_t* value =
                    element.second.get_ptr<const uint64_t*>();
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
