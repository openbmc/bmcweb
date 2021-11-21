#include <cstddef>
#include <array>
#include <utility>
#include <boost/circular_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "../../http/app_class_decl.hpp"
using crow::App;
#include "event_service.hpp"

namespace redfish
{

void requestRoutesEventService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/")
        .privileges(redfish::privileges::getEventService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#EventService.v1_5_0.EventService"},
                    {"Id", "EventService"},
                    {"Name", "Event Service"},
                    {"Subscriptions",
                     {{"@odata.id", "/redfish/v1/EventService/Subscriptions"}}},
                    {"Actions",
                     {{"#EventService.SubmitTestEvent",
                       {{"target", "/redfish/v1/EventService/Actions/"
                                   "EventService.SubmitTestEvent"}}}}},
                    {"@odata.id", "/redfish/v1/EventService"}};

                const persistent_data::EventServiceConfig eventServiceConfig =
                    persistent_data::EventServiceStore::getInstance()
                        .getEventServiceConfig();

                asyncResp->res.jsonValue["Status"]["State"] =
                    (eventServiceConfig.enabled ? "Enabled" : "Disabled");
                asyncResp->res.jsonValue["ServiceEnabled"] =
                    eventServiceConfig.enabled;
                asyncResp->res.jsonValue["DeliveryRetryAttempts"] =
                    eventServiceConfig.retryAttempts;
                asyncResp->res.jsonValue["DeliveryRetryIntervalSeconds"] =
                    eventServiceConfig.retryTimeoutInterval;
                asyncResp->res.jsonValue["EventFormatTypes"] =
                    supportedEvtFormatTypes;
                asyncResp->res.jsonValue["RegistryPrefixes"] =
                    supportedRegPrefixes;
                asyncResp->res.jsonValue["ResourceTypes"] =
                    supportedResourceTypes;

                nlohmann::json supportedSSEFilters = {
                    {"EventFormatType", true},        {"MessageId", true},
                    {"MetricReportDefinition", true}, {"RegistryPrefix", true},
                    {"OriginResource", false},        {"ResourceType", false}};

                asyncResp->res.jsonValue["SSEFilterPropertiesSupported"] =
                    supportedSSEFilters;
            });

    BMCWEB_ROUTE(app, "/redfish/v1/EventService/")
        .privileges(redfish::privileges::patchEventService)
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            {
                std::optional<bool> serviceEnabled;
                std::optional<uint32_t> retryAttemps;
                std::optional<uint32_t> retryInterval;

                if (!json_util::readJson(
                        req, asyncResp->res, "ServiceEnabled", serviceEnabled,
                        "DeliveryRetryAttempts", retryAttemps,
                        "DeliveryRetryIntervalSeconds", retryInterval))
                {
                    return;
                }

                persistent_data::EventServiceConfig eventServiceConfig =
                    persistent_data::EventServiceStore::getInstance()
                        .getEventServiceConfig();

                if (serviceEnabled)
                {
                    eventServiceConfig.enabled = *serviceEnabled;
                }

                if (retryAttemps)
                {
                    // Supported range [1-3]
                    if ((*retryAttemps < 1) || (*retryAttemps > 3))
                    {
                        messages::queryParameterOutOfRange(
                            asyncResp->res, std::to_string(*retryAttemps),
                            "DeliveryRetryAttempts", "[1-3]");
                    }
                    else
                    {
                        eventServiceConfig.retryAttempts = *retryAttemps;
                    }
                }

                if (retryInterval)
                {
                    // Supported range [30 - 180]
                    if ((*retryInterval < 30) || (*retryInterval > 180))
                    {
                        messages::queryParameterOutOfRange(
                            asyncResp->res, std::to_string(*retryInterval),
                            "DeliveryRetryIntervalSeconds", "[30-180]");
                    }
                    else
                    {
                        eventServiceConfig.retryTimeoutInterval =
                            *retryInterval;
                    }
                }

                EventServiceManager::getInstance().setEventServiceConfig(
                    eventServiceConfig);
            });
}

void requestRoutesSubmitTestEvent(App& app)
{

    BMCWEB_ROUTE(
        app, "/redfish/v1/EventService/Actions/EventService.SubmitTestEvent/")
        .privileges(redfish::privileges::postEventService)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                EventServiceManager::getInstance().sendTestEventLog();
                asyncResp->res.result(boost::beast::http::status::no_content);
            });
}

void requestRoutesEventDestinationCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions")
        .privileges(redfish::privileges::getEventDestinationCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#EventDestinationCollection.EventDestinationCollection"},
                    {"@odata.id", "/redfish/v1/EventService/Subscriptions"},
                    {"Name", "Event Destination Collections"}};

                nlohmann::json& memberArray =
                    asyncResp->res.jsonValue["Members"];

                std::vector<std::string> subscripIds =
                    EventServiceManager::getInstance().getAllIDs();
                memberArray = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] =
                    subscripIds.size();

                for (const std::string& id : subscripIds)
                {
                    memberArray.push_back(
                        {{"@odata.id",
                          "/redfish/v1/EventService/Subscriptions/" + id}});
                }
            });
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/")
        .privileges(redfish::privileges::postEventDestinationCollection)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (EventServiceManager::getInstance()
                        .getNumberOfSubscriptions() >= maxNoOfSubscriptions)
                {
                    messages::eventSubscriptionLimitExceeded(asyncResp->res);
                    return;
                }
                std::string destUrl;
                std::string protocol;
                std::optional<std::string> context;
                std::optional<std::string> subscriptionType;
                std::optional<std::string> eventFormatType2;
                std::optional<std::string> retryPolicy;
                std::optional<std::vector<std::string>> msgIds;
                std::optional<std::vector<std::string>> regPrefixes;
                std::optional<std::vector<std::string>> resTypes;
                std::optional<std::vector<nlohmann::json>> headers;
                std::optional<std::vector<nlohmann::json>> mrdJsonArray;

                if (!json_util::readJson(
                        req, asyncResp->res, "Destination", destUrl, "Context",
                        context, "Protocol", protocol, "SubscriptionType",
                        subscriptionType, "EventFormatType", eventFormatType2,
                        "HttpHeaders", headers, "RegistryPrefixes", regPrefixes,
                        "MessageIds", msgIds, "DeliveryRetryPolicy",
                        retryPolicy, "MetricReportDefinitions", mrdJsonArray,
                        "ResourceTypes", resTypes))
                {
                    return;
                }

                if (regPrefixes && msgIds)
                {
                    if (regPrefixes->size() && msgIds->size())
                    {
                        messages::mutualExclusiveProperties(
                            asyncResp->res, "RegistryPrefixes", "MessageIds");
                        return;
                    }
                }

                // Validate the URL using regex expression
                // Format: <protocol>://<host>:<port>/<uri>
                // protocol: http/https
                // host: Exclude ' ', ':', '#', '?'
                // port: Empty or numeric value with ':' separator.
                // uri: Start with '/' and Exclude '#', ' '
                //      Can include query params(ex: '/event?test=1')
                // TODO: Need to validate hostname extensively(as per rfc)
                const std::regex urlRegex(
                    "(http|https)://([^/\\x20\\x3f\\x23\\x3a]+):?([0-9]*)(/"
                    "([^\\x20\\x23\\x3f]*\\x3f?([^\\x20\\x23\\x3f])*)?)");
                std::cmatch match;
                if (!std::regex_match(destUrl.c_str(), match, urlRegex))
                {
                    messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                       "Destination");
                    return;
                }

                std::string uriProto =
                    std::string(match[1].first, match[1].second);
                if (uriProto == "http")
                {
#ifndef BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING
                    messages::propertyValueFormatError(asyncResp->res, destUrl,
                                                       "Destination");
                    return;
#endif
                }

                std::string host = std::string(match[2].first, match[2].second);
                std::string port = std::string(match[3].first, match[3].second);
                std::string path = std::string(match[4].first, match[4].second);
                if (port.empty())
                {
                    if (uriProto == "http")
                    {
                        port = "80";
                    }
                    else
                    {
                        port = "443";
                    }
                }
                if (path.empty())
                {
                    path = "/";
                }

                std::shared_ptr<Subscription> subValue =
                    std::make_shared<Subscription>(host, port, path, uriProto);

                subValue->destinationUrl = destUrl;

                if (subscriptionType)
                {
                    if (*subscriptionType != "RedfishEvent")
                    {
                        messages::propertyValueNotInList(asyncResp->res,
                                                         *subscriptionType,
                                                         "SubscriptionType");
                        return;
                    }
                    subValue->subscriptionType = *subscriptionType;
                }
                else
                {
                    subValue->subscriptionType = "RedfishEvent"; // Default
                }

                if (protocol != "Redfish")
                {
                    messages::propertyValueNotInList(asyncResp->res, protocol,
                                                     "Protocol");
                    return;
                }
                subValue->protocol = protocol;

                if (eventFormatType2)
                {
                    if (std::find(supportedEvtFormatTypes.begin(),
                                  supportedEvtFormatTypes.end(),
                                  *eventFormatType2) ==
                        supportedEvtFormatTypes.end())
                    {
                        messages::propertyValueNotInList(asyncResp->res,
                                                         *eventFormatType2,
                                                         "EventFormatType");
                        return;
                    }
                    subValue->eventFormatType = *eventFormatType2;
                }
                else
                {
                    // If not specified, use default "Event"
                    subValue->eventFormatType = "Event";
                }

                if (context)
                {
                    subValue->customText = *context;
                }

                if (headers)
                {
                    for (const nlohmann::json& headerChunk : *headers)
                    {
                        for (const auto& item : headerChunk.items())
                        {
                            const std::string* value =
                                item.value().get_ptr<const std::string*>();
                            if (value == nullptr)
                            {
                                messages::propertyValueFormatError(
                                    asyncResp->res, item.value().dump(2, true),
                                    "HttpHeaders/" + item.key());
                                return;
                            }
                            subValue->httpHeaders.set(item.key(), *value);
                        }
                    }
                }

                if (regPrefixes)
                {
                    for (const std::string& it : *regPrefixes)
                    {
                        if (std::find(supportedRegPrefixes.begin(),
                                      supportedRegPrefixes.end(),
                                      it) == supportedRegPrefixes.end())
                        {
                            messages::propertyValueNotInList(
                                asyncResp->res, it, "RegistryPrefixes");
                            return;
                        }
                    }
                    subValue->registryPrefixes = *regPrefixes;
                }

                if (resTypes)
                {
                    for (const std::string& it : *resTypes)
                    {
                        if (std::find(supportedResourceTypes.begin(),
                                      supportedResourceTypes.end(),
                                      it) == supportedResourceTypes.end())
                        {
                            messages::propertyValueNotInList(asyncResp->res, it,
                                                             "ResourceTypes");
                            return;
                        }
                    }
                    subValue->resourceTypes = *resTypes;
                }

                if (msgIds)
                {
                    std::vector<std::string> registryPrefix;

                    // If no registry prefixes are mentioned, consider all
                    // supported prefixes
                    if (subValue->registryPrefixes.empty())
                    {
                        registryPrefix.assign(supportedRegPrefixes.begin(),
                                              supportedRegPrefixes.end());
                    }
                    else
                    {
                        registryPrefix = subValue->registryPrefixes;
                    }

                    for (const std::string& id : *msgIds)
                    {
                        bool validId = false;

                        // Check for Message ID in each of the selected Registry
                        for (const std::string& it : registryPrefix)
                        {
                            const boost::beast::span<
                                const redfish::message_registries::MessageEntry>
                                registry = redfish::message_registries::
                                    getRegistryFromPrefix(it);

                            if (std::any_of(
                                    registry.cbegin(), registry.cend(),
                                    [&id](const redfish::message_registries::
                                              MessageEntry& messageEntry) {
                                        return !id.compare(messageEntry.first);
                                    }))
                            {
                                validId = true;
                                break;
                            }
                        }

                        if (!validId)
                        {
                            messages::propertyValueNotInList(asyncResp->res, id,
                                                             "MessageIds");
                            return;
                        }
                    }

                    subValue->registryMsgIds = *msgIds;
                }

                if (retryPolicy)
                {
                    if (std::find(supportedRetryPolicies.begin(),
                                  supportedRetryPolicies.end(),
                                  *retryPolicy) == supportedRetryPolicies.end())
                    {
                        messages::propertyValueNotInList(asyncResp->res,
                                                         *retryPolicy,
                                                         "DeliveryRetryPolicy");
                        return;
                    }
                    subValue->retryPolicy = *retryPolicy;
                }
                else
                {
                    // Default "TerminateAfterRetries"
                    subValue->retryPolicy = "TerminateAfterRetries";
                }

                if (mrdJsonArray)
                {
                    for (nlohmann::json& mrdObj : *mrdJsonArray)
                    {
                        std::string mrdUri;
                        if (json_util::getValueFromJsonObject(
                                mrdObj, "@odata.id", mrdUri))
                        {
                            subValue->metricReportDefinitions.emplace_back(
                                mrdUri);
                        }
                        else
                        {
                            messages::propertyValueFormatError(
                                asyncResp->res,
                                mrdObj.dump(
                                    2, ' ', true,
                                    nlohmann::json::error_handler_t::replace),
                                "MetricReportDefinitions");
                            return;
                        }
                    }
                }

                std::string id =
                    EventServiceManager::getInstance().addSubscription(
                        subValue);
                if (id.empty())
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                messages::created(asyncResp->res);
                asyncResp->res.addHeader(
                    "Location", "/redfish/v1/EventService/Subscriptions/" + id);
            });
}

void requestRoutesEventDestination(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        .privileges(redfish::privileges::getEventDestination)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                std::shared_ptr<Subscription> subValue =
                    EventServiceManager::getInstance().getSubscription(param);
                if (subValue == nullptr)
                {
                    asyncResp->res.result(
                        boost::beast::http::status::not_found);
                    return;
                }
                const std::string& id = param;

                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#EventDestination.v1_7_0.EventDestination"},
                    {"Protocol", "Redfish"}};
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/EventService/Subscriptions/" + id;
                asyncResp->res.jsonValue["Id"] = id;
                asyncResp->res.jsonValue["Name"] = "Event Destination " + id;
                asyncResp->res.jsonValue["Destination"] =
                    subValue->destinationUrl;
                asyncResp->res.jsonValue["Context"] = subValue->customText;
                asyncResp->res.jsonValue["SubscriptionType"] =
                    subValue->subscriptionType;
                asyncResp->res.jsonValue["HttpHeaders"] =
                    nlohmann::json::array();
                asyncResp->res.jsonValue["EventFormatType"] =
                    subValue->eventFormatType;
                asyncResp->res.jsonValue["RegistryPrefixes"] =
                    subValue->registryPrefixes;
                asyncResp->res.jsonValue["ResourceTypes"] =
                    subValue->resourceTypes;

                asyncResp->res.jsonValue["MessageIds"] =
                    subValue->registryMsgIds;
                asyncResp->res.jsonValue["DeliveryRetryPolicy"] =
                    subValue->retryPolicy;

                std::vector<nlohmann::json> mrdJsonArray;
                for (const auto& mdrUri : subValue->metricReportDefinitions)
                {
                    mrdJsonArray.push_back({{"@odata.id", mdrUri}});
                }
                asyncResp->res.jsonValue["MetricReportDefinitions"] =
                    mrdJsonArray;
            });
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        // The below privilege is wrong, it should be ConfigureManager OR
        // ConfigureSelf
        // https://github.com/openbmc/bmcweb/issues/220
        //.privileges(redfish::privileges::patchEventDestination)
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                std::shared_ptr<Subscription> subValue =
                    EventServiceManager::getInstance().getSubscription(param);
                if (subValue == nullptr)
                {
                    asyncResp->res.result(
                        boost::beast::http::status::not_found);
                    return;
                }

                std::optional<std::string> context;
                std::optional<std::string> retryPolicy;
                std::optional<std::vector<nlohmann::json>> headers;

                if (!json_util::readJson(req, asyncResp->res, "Context",
                                         context, "DeliveryRetryPolicy",
                                         retryPolicy, "HttpHeaders", headers))
                {
                    return;
                }

                if (context)
                {
                    subValue->customText = *context;
                }

                if (headers)
                {
                    boost::beast::http::fields fields;
                    for (const nlohmann::json& headerChunk : *headers)
                    {
                        for (auto& it : headerChunk.items())
                        {
                            const std::string* value =
                                it.value().get_ptr<const std::string*>();
                            if (value == nullptr)
                            {
                                messages::propertyValueFormatError(
                                    asyncResp->res,
                                    it.value().dump(2, ' ', true),
                                    "HttpHeaders/" + it.key());
                                return;
                            }
                            fields.set(it.key(), *value);
                        }
                    }
                    subValue->httpHeaders = fields;
                }

                if (retryPolicy)
                {
                    if (std::find(supportedRetryPolicies.begin(),
                                  supportedRetryPolicies.end(),
                                  *retryPolicy) == supportedRetryPolicies.end())
                    {
                        messages::propertyValueNotInList(asyncResp->res,
                                                         *retryPolicy,
                                                         "DeliveryRetryPolicy");
                        return;
                    }
                    subValue->retryPolicy = *retryPolicy;
                    subValue->updateRetryPolicy();
                }

                EventServiceManager::getInstance().updateSubscriptionData();
            });
    BMCWEB_ROUTE(app, "/redfish/v1/EventService/Subscriptions/<str>/")
        // The below privilege is wrong, it should be ConfigureManager OR
        // ConfigureSelf
        // https://github.com/openbmc/bmcweb/issues/220
        //.privileges(redfish::privileges::deleteEventDestination)
        .privileges({{"ConfigureManager"}})
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& param) {
                if (!EventServiceManager::getInstance().isSubscriptionExist(
                        param))
                {
                    asyncResp->res.result(
                        boost::beast::http::status::not_found);
                    return;
                }
                EventServiceManager::getInstance().deleteSubscription(param);
            });
}

// from event_service_manager

namespace message_registries
{
const Message*
    getMsgFromRegistry(const std::string& messageKey,
                       const boost::beast::span<const MessageEntry>& registry)
{
    boost::beast::span<const MessageEntry>::const_iterator messageIt =
        std::find_if(registry.cbegin(), registry.cend(),
                     [&messageKey](const MessageEntry& messageEntry) {
                         return !messageKey.compare(messageEntry.first);
                     });
    if (messageIt != registry.cend())
    {
        return &messageIt->second;
    }

    return nullptr;
}

const Message* formatMessage(const std::string_view& messageID)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, boost::is_any_of("."));
    if (fields.size() != 4)
    {
        return nullptr;
    }
    std::string& registryName = fields[0];
    std::string& messageKey = fields[3];

    // Find the right registry and check it for the MessageKey
    return getMsgFromRegistry(messageKey, getRegistryFromPrefix(registryName));
}

}

namespace event_log
{

bool getUniqueEntryID(const std::string& logEntry, std::string& entryID,
                             const bool firstEntry)
{
    static time_t prevTs = 0;
    static int index = 0;
    if (firstEntry)
    {
        prevTs = 0;
    }

    // Get the entry timestamp
    std::time_t curTs = 0;
    std::tm timeStruct = {};
    std::istringstream entryStream(logEntry);
    if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
    {
        curTs = std::mktime(&timeStruct);
        if (curTs == -1)
        {
            return false;
        }
    }
    // If the timestamp isn't unique, increment the index
    index = (curTs == prevTs) ? index + 1 : 0;

    // Save the timestamp
    prevTs = curTs;

    entryID = std::to_string(curTs);
    if (index > 0)
    {
        entryID += "_" + std::to_string(index);
    }
    return true;
}

int getEventLogParams(const std::string& logEntry,
                             std::string& timestamp, std::string& messageID,
                             std::vector<std::string>& messageArgs)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(' ');
    if (space == std::string::npos)
    {
        return -EINVAL;
    }
    timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(' ', space);
    if (entryStart == std::string::npos)
    {
        return -EINVAL;
    }
    std::string_view entry(logEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> logEntryFields;
    boost::split(logEntryFields, entry, boost::is_any_of(","),
                 boost::token_compress_on);
    // We need at least a MessageId to be valid
    if (logEntryFields.size() < 1)
    {
        return -EINVAL;
    }
    messageID = logEntryFields[0];

    // Get the MessageArgs from the log if there are any
    if (logEntryFields.size() > 1)
    {
        std::string& messageArgsStart = logEntryFields[1];
        // If the first string is empty, assume there are no MessageArgs
        if (!messageArgsStart.empty())
        {
            messageArgs.assign(logEntryFields.begin() + 1,
                               logEntryFields.end());
        }
    }

    return 0;
}

void getRegistryAndMessageKey(const std::string& messageID,
                                     std::string& registryName,
                                     std::string& messageKey)
{
    // Redfish MessageIds are in the form
    // RegistryName.MajorVersion.MinorVersion.MessageKey, so parse it to find
    // the right Message
    std::vector<std::string> fields;
    fields.reserve(4);
    boost::split(fields, messageID, boost::is_any_of("."));
    if (fields.size() == 4)
    {
        registryName = fields[0];
        messageKey = fields[3];
    }
}

int formatEventLogEntry(const std::string& logEntryID,
                               const std::string& messageID,
                               const std::vector<std::string>& messageArgs,
                               std::string timestamp,
                               const std::string& customText,
                               nlohmann::json& logEntryJson)
{
    // Get the Message from the MessageRegistry
    const message_registries::Message* message =
        message_registries::formatMessage(messageID);

    std::string msg;
    std::string severity;
    if (message != nullptr)
    {
        msg = message->message;
        severity = message->severity;
    }

    // Fill the MessageArgs into the Message
    int i = 0;
    for (const std::string& messageArg : messageArgs)
    {
        std::string argStr = "%" + std::to_string(++i);
        size_t argPos = msg.find(argStr);
        if (argPos != std::string::npos)
        {
            msg.replace(argPos, argStr.length(), messageArg);
        }
    }

    // Get the Created time from the timestamp. The log timestamp is in
    // RFC3339 format which matches the Redfish format except for the
    // fractional seconds between the '.' and the '+', so just remove them.
    std::size_t dot = timestamp.find_first_of('.');
    std::size_t plus = timestamp.find_first_of('+');
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    // Fill in the log entry with the gathered data
    logEntryJson = {{"EventId", logEntryID},
                    {"EventType", "Event"},
                    {"Severity", std::move(severity)},
                    {"Message", std::move(msg)},
                    {"MessageId", messageID},
                    {"MessageArgs", messageArgs},
                    {"EventTimestamp", std::move(timestamp)},
                    {"Context", customText}};
    return 0;
}

}

bool isFilterQuerySpecialChar(char c)
{
    switch (c)
    {
        case '(':
        case ')':
        case '\'':
            return true;
        default:
            return false;
    }
}

bool readSSEQueryParams(std::string sseFilter, std::string& formatType,
                       std::vector<std::string>& messageIds,
                       std::vector<std::string>& registryPrefixes,
                       std::vector<std::string>& metricReportDefinitions)
{
    sseFilter.erase(std::remove_if(sseFilter.begin(), sseFilter.end(),
                                   isFilterQuerySpecialChar),
                    sseFilter.end());

    std::vector<std::string> result;
    boost::split(result, sseFilter, boost::is_any_of(" "),
                 boost::token_compress_on);

    BMCWEB_LOG_DEBUG << "No of tokens in SEE query: " << result.size();

    constexpr uint8_t divisor = 4;
    constexpr uint8_t minTokenSize = 3;
    if (result.size() % divisor != minTokenSize)
    {
        BMCWEB_LOG_ERROR << "Invalid SSE filter specified.";
        return false;
    }

    for (std::size_t i = 0; i < result.size(); i += divisor)
    {
        std::string& key = result[i];
        std::string& op = result[i + 1];
        std::string& value = result[i + 2];

        if ((i + minTokenSize) < result.size())
        {
            std::string& separator = result[i + minTokenSize];
            // SSE supports only "or" and "and" in query params.
            if ((separator != "or") && (separator != "and"))
            {
                BMCWEB_LOG_ERROR
                    << "Invalid group operator in SSE query parameters";
                return false;
            }
        }

        // SSE supports only "eq" as per spec.
        if (op != "eq")
        {
            BMCWEB_LOG_ERROR
                << "Invalid assignment operator in SSE query parameters";
            return false;
        }

        BMCWEB_LOG_DEBUG << key << " : " << value;
        if (key == "EventFormatType")
        {
            formatType = value;
        }
        else if (key == "MessageId")
        {
            messageIds.push_back(value);
        }
        else if (key == "RegistryPrefix")
        {
            registryPrefixes.push_back(value);
        }
        else if (key == "MetricReportDefinition")
        {
            metricReportDefinitions.push_back(value);
        }
        else
        {
            BMCWEB_LOG_ERROR << "Invalid property(" << key
                             << ")in SSE filter query.";
            return false;
        }
    }
    return true;
}

Subscription::Subscription(const std::string& inHost, const std::string& inPort,
                 const std::string& inPath, const std::string& inUriProto) :
        eventSeqNum(1),
        host(inHost), port(inPort), path(inPath), uriProto(inUriProto)
{
    conn = std::make_shared<crow::HttpClient>(
        crow::connections::systemBus->get_io_context(), id, host, port,
        path, httpHeaders);
}

Subscription::Subscription(const std::shared_ptr<boost::beast::tcp_stream>& adaptor) :
    eventSeqNum(1)
{
    sseConn = std::make_shared<crow::ServerSentEvents>(adaptor);
}

void Subscription::sendEvent(const std::string& msg)
{
    if (conn != nullptr)
    {
        conn->sendData(msg);
        this->eventSeqNum++;
    }

    if (sseConn != nullptr)
    {
        sseConn->sendData(eventSeqNum, msg);
    }
}

void Subscription::sendTestEventLog()
{
    nlohmann::json logEntryArray;
    logEntryArray.push_back({});
    nlohmann::json& logEntryJson = logEntryArray.back();

    logEntryJson = {
        {"EventId", "TestID"},
        {"EventType", "Event"},
        {"Severity", "OK"},
        {"Message", "Generated test event"},
        {"MessageId", "OpenBMC.0.2.TestEventLog"},
        {"MessageArgs", nlohmann::json::array()},
        {"EventTimestamp", crow::utility::getDateTimeOffsetNow().first},
        {"Context", customText}};

    nlohmann::json msg = {{"@odata.type", "#Event.v1_4_0.Event"},
                            {"Id", std::to_string(eventSeqNum)},
                            {"Name", "Event Log"},
                            {"Events", logEntryArray}};

    this->sendEvent(
        msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace));
}

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
void Subscription::filterAndSendEventLogs(
    const std::vector<EventLogObjectsType>& eventRecords)
{
    nlohmann::json logEntryArray;
    for (const EventLogObjectsType& logEntry : eventRecords)
    {
        const std::string& idStr = std::get<0>(logEntry);
        const std::string& timestamp = std::get<1>(logEntry);
        const std::string& messageID = std::get<2>(logEntry);
        const std::string& registryName = std::get<3>(logEntry);
        const std::string& messageKey = std::get<4>(logEntry);
        const std::vector<std::string>& messageArgs = std::get<5>(logEntry);

        // If registryPrefixes list is empty, don't filter events
        // send everything.
        if (registryPrefixes.size())
        {
            auto obj = std::find(registryPrefixes.begin(),
                                    registryPrefixes.end(), registryName);
            if (obj == registryPrefixes.end())
            {
                continue;
            }
        }

        // If registryMsgIds list is empty, don't filter events
        // send everything.
        if (registryMsgIds.size())
        {
            auto obj = std::find(registryMsgIds.begin(),
                                    registryMsgIds.end(), messageKey);
            if (obj == registryMsgIds.end())
            {
                continue;
            }
        }

        logEntryArray.push_back({});
        nlohmann::json& bmcLogEntry = logEntryArray.back();
        if (event_log::formatEventLogEntry(idStr, messageID, messageArgs,
                                            timestamp, customText,
                                            bmcLogEntry) != 0)
        {
            BMCWEB_LOG_DEBUG << "Read eventLog entry failed";
            continue;
        }
    }

    if (logEntryArray.size() < 1)
    {
        BMCWEB_LOG_DEBUG << "No log entries available to be transferred.";
        return;
    }

    nlohmann::json msg = {{"@odata.type", "#Event.v1_4_0.Event"},
                            {"Id", std::to_string(eventSeqNum)},
                            {"Name", "Event Log"},
                            {"Events", logEntryArray}};

    this->sendEvent(
        msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace));
}
#endif

void Subscription::filterAndSendReports(const std::string& id2,
                              const std::string& readingsTs,
                            const ReadingsObjType& readings)
{
    std::string metricReportDef =
        "/redfish/v1/TelemetryService/MetricReportDefinitions/" + id2;

    // Empty list means no filter. Send everything.
    if (metricReportDefinitions.size())
    {
        if (std::find(metricReportDefinitions.begin(),
                        metricReportDefinitions.end(),
                        metricReportDef) == metricReportDefinitions.end())
        {
            return;
        }
    }

    nlohmann::json metricValuesArray = nlohmann::json::array();
    for (const auto& it : readings)
    {
        metricValuesArray.push_back({});
        nlohmann::json& entry = metricValuesArray.back();

        auto& [id, property, value, timestamp] = it;

        entry = {{"MetricId", id},
                    {"MetricProperty", property},
                    {"MetricValue", std::to_string(value)},
                    {"Timestamp", crow::utility::getDateTime(timestamp)}};
    }

    nlohmann::json msg = {
        {"@odata.id", "/redfish/v1/TelemetryService/MetricReports/" + id},
        {"@odata.type", "#MetricReport.v1_3_0.MetricReport"},
        {"Id", id2},
        {"Name", id2},
        {"Timestamp", readingsTs},
        {"MetricReportDefinition", {{"@odata.id", metricReportDef}}},
        {"MetricValues", metricValuesArray}};

    this->sendEvent(
        msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace));
}

void Subscription::updateRetryConfig(const uint32_t retryAttempts,
                        const uint32_t retryTimeoutInterval)
{
    if (conn != nullptr)
    {
        conn->setRetryConfig(retryAttempts, retryTimeoutInterval);
    }
}

void Subscription::updateRetryPolicy()
{
    if (conn != nullptr)
    {
        conn->setRetryPolicy(retryPolicy);
    }
}

uint64_t Subscription::getEventSeqNum()
{
    return eventSeqNum;
}

EventServiceManager::EventServiceManager()
{
    // Load config from persist store.
    initConfig();
}

EventServiceManager& EventServiceManager::getInstance()
{
    static EventServiceManager handler;
    return handler;
}

void EventServiceManager::initConfig()
{
    loadOldBehavior();

    persistent_data::EventServiceConfig eventServiceConfig =
        persistent_data::EventServiceStore::getInstance()
            .getEventServiceConfig();

    serviceEnabled = eventServiceConfig.enabled;
    retryAttempts = eventServiceConfig.retryAttempts;
    retryTimeoutInterval = eventServiceConfig.retryTimeoutInterval;

    for (const auto& it : persistent_data::EventServiceStore::getInstance()
                                .subscriptionsConfigMap)
    {
        std::shared_ptr<persistent_data::UserSubscription> newSub =
            it.second;

        std::string host;
        std::string urlProto;
        std::string port;
        std::string path;
        bool status = validateAndSplitUrl(newSub->destinationUrl, urlProto,
                                            host, port, path);

        if (!status)
        {
            BMCWEB_LOG_ERROR
                << "Failed to validate and split destination url";
            continue;
        }
        std::shared_ptr<Subscription> subValue =
            std::make_shared<Subscription>(host, port, path, urlProto);

        subValue->id = newSub->id;
        subValue->destinationUrl = newSub->destinationUrl;
        subValue->protocol = newSub->protocol;
        subValue->retryPolicy = newSub->retryPolicy;
        subValue->customText = newSub->customText;
        subValue->eventFormatType = newSub->eventFormatType;
        subValue->subscriptionType = newSub->subscriptionType;
        subValue->registryMsgIds = newSub->registryMsgIds;
        subValue->registryPrefixes = newSub->registryPrefixes;
        subValue->resourceTypes = newSub->resourceTypes;
        subValue->httpHeaders = newSub->httpHeaders;
        subValue->metricReportDefinitions = newSub->metricReportDefinitions;

        if (subValue->id.empty())
        {
            BMCWEB_LOG_ERROR << "Failed to add subscription";
        }
        subscriptionsMap.insert(std::pair(subValue->id, subValue));

        updateNoOfSubscribersCount();

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
        if (lastEventTStr.empty())
        {
            cacheLastEventTimestamp();
        }
#endif
        // Update retry configuration.
        subValue->updateRetryConfig(retryAttempts, retryTimeoutInterval);
        subValue->updateRetryPolicy();
    }
    return;
}

void EventServiceManager::loadOldBehavior()
{
    std::ifstream eventConfigFile(eventServiceFile);
    if (!eventConfigFile.good())
    {
        BMCWEB_LOG_DEBUG << "Old eventService config not exist";
        return;
    }
    auto jsonData = nlohmann::json::parse(eventConfigFile, nullptr, false);
    if (jsonData.is_discarded())
    {
        BMCWEB_LOG_ERROR << "Old eventService config parse error.";
        return;
    }

    for (const auto& item : jsonData.items())
    {
        if (item.key() == "Configuration")
        {
            persistent_data::EventServiceStore::getInstance()
                .getEventServiceConfig()
                .fromJson(item.value());
        }
        else if (item.key() == "Subscriptions")
        {
            for (const auto& elem : item.value())
            {
                std::shared_ptr<persistent_data::UserSubscription>
                    newSubscription =
                        persistent_data::UserSubscription::fromJson(elem,
                                                                    true);
                if (newSubscription == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Problem reading subscription "
                                        "from old persistent store";
                    continue;
                }

                std::uniform_int_distribution<uint32_t> dist(0);
                bmcweb::OpenSSLGenerator gen;

                std::string id;

                int retry = 3;
                while (retry)
                {
                    id = std::to_string(dist(gen));
                    if (gen.error())
                    {
                        retry = 0;
                        break;
                    }
                    newSubscription->id = id;
                    auto inserted =
                        persistent_data::EventServiceStore::getInstance()
                            .subscriptionsConfigMap.insert(
                                std::pair(id, newSubscription));
                    if (inserted.second)
                    {
                        break;
                    }
                    --retry;
                }

                if (retry <= 0)
                {
                    BMCWEB_LOG_ERROR
                        << "Failed to generate random number from old "
                            "persistent store";
                    continue;
                }
            }
        }

        persistent_data::getConfig().writeData();
        std::remove(eventServiceFile);
        BMCWEB_LOG_DEBUG << "Remove old eventservice config";
    }
}

void EventServiceManager::updateSubscriptionData()
{
    persistent_data::EventServiceStore::getInstance()
        .eventServiceConfig.enabled = serviceEnabled;
    persistent_data::EventServiceStore::getInstance()
        .eventServiceConfig.retryAttempts = retryAttempts;
    persistent_data::EventServiceStore::getInstance()
        .eventServiceConfig.retryTimeoutInterval = retryTimeoutInterval;

    persistent_data::getConfig().writeData();
}

void EventServiceManager::setEventServiceConfig(const persistent_data::EventServiceConfig& cfg)
{
    bool updateConfig = false;
    bool updateRetryCfg = false;

    if (serviceEnabled != cfg.enabled)
    {
        serviceEnabled = cfg.enabled;
        if (serviceEnabled && noOfMetricReportSubscribers)
        {
            registerMetricReportSignal();
        }
        else
        {
            unregisterMetricReportSignal();
        }
        updateConfig = true;
    }

    if (retryAttempts != cfg.retryAttempts)
    {
        retryAttempts = cfg.retryAttempts;
        updateConfig = true;
        updateRetryCfg = true;
    }

    if (retryTimeoutInterval != cfg.retryTimeoutInterval)
    {
        retryTimeoutInterval = cfg.retryTimeoutInterval;
        updateConfig = true;
        updateRetryCfg = true;
    }

    if (updateConfig)
    {
        updateSubscriptionData();
    }

    if (updateRetryCfg)
    {
        // Update the changed retry config to all subscriptions
        for (const auto& it :
                EventServiceManager::getInstance().subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            entry->updateRetryConfig(retryAttempts, retryTimeoutInterval);
        }
    }
}

void EventServiceManager::updateNoOfSubscribersCount()
{
    size_t eventLogSubCount = 0;
    size_t metricReportSubCount = 0;
    for (const auto& it : subscriptionsMap)
    {
        std::shared_ptr<Subscription> entry = it.second;
        if (entry->eventFormatType == eventFormatType)
        {
            eventLogSubCount++;
        }
        else if (entry->eventFormatType == metricReportFormatType)
        {
            metricReportSubCount++;
        }
    }

    noOfEventLogSubscribers = eventLogSubCount;
    if (noOfMetricReportSubscribers != metricReportSubCount)
    {
        noOfMetricReportSubscribers = metricReportSubCount;
        if (noOfMetricReportSubscribers)
        {
            registerMetricReportSignal();
        }
        else
        {
            unregisterMetricReportSignal();
        }
    }
}

std::shared_ptr<Subscription> EventServiceManager::getSubscription(const std::string& id)
{
    auto obj = subscriptionsMap.find(id);
    if (obj == subscriptionsMap.end())
    {
        BMCWEB_LOG_ERROR << "No subscription exist with ID:" << id;
        return nullptr;
    }
    std::shared_ptr<Subscription> subValue = obj->second;
    return subValue;
}

std::string EventServiceManager::addSubscription(const std::shared_ptr<Subscription>& subValue,
                                const bool updateFile)
{

    std::uniform_int_distribution<uint32_t> dist(0);
    bmcweb::OpenSSLGenerator gen;

    std::string id;

    int retry = 3;
    while (retry)
    {
        id = std::to_string(dist(gen));
        if (gen.error())
        {
            retry = 0;
            break;
        }
        auto inserted = subscriptionsMap.insert(std::pair(id, subValue));
        if (inserted.second)
        {
            break;
        }
        --retry;
    }

    if (retry <= 0)
    {
        BMCWEB_LOG_ERROR << "Failed to generate random number";
        return "";
    }

    std::shared_ptr<persistent_data::UserSubscription> newSub =
        std::make_shared<persistent_data::UserSubscription>();
    newSub->id = id;
    newSub->destinationUrl = subValue->destinationUrl;
    newSub->protocol = subValue->protocol;
    newSub->retryPolicy = subValue->retryPolicy;
    newSub->customText = subValue->customText;
    newSub->eventFormatType = subValue->eventFormatType;
    newSub->subscriptionType = subValue->subscriptionType;
    newSub->registryMsgIds = subValue->registryMsgIds;
    newSub->registryPrefixes = subValue->registryPrefixes;
    newSub->resourceTypes = subValue->resourceTypes;
    newSub->httpHeaders = subValue->httpHeaders;
    newSub->metricReportDefinitions = subValue->metricReportDefinitions;
    persistent_data::EventServiceStore::getInstance()
        .subscriptionsConfigMap.emplace(newSub->id, newSub);

    updateNoOfSubscribersCount();

    if (updateFile)
    {
        updateSubscriptionData();
    }

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    if (lastEventTStr.empty())
    {
        cacheLastEventTimestamp();
    }
#endif
    // Update retry configuration.
    subValue->updateRetryConfig(retryAttempts, retryTimeoutInterval);
    subValue->updateRetryPolicy();

    return id;
}

bool EventServiceManager::isSubscriptionExist(const std::string& id)
{
    auto obj = subscriptionsMap.find(id);
    if (obj == subscriptionsMap.end())
    {
        return false;
    }
    return true;
}

void EventServiceManager::deleteSubscription(const std::string& id)
{
    auto obj = subscriptionsMap.find(id);
    if (obj != subscriptionsMap.end())
    {
        subscriptionsMap.erase(obj);
        auto obj2 = persistent_data::EventServiceStore::getInstance()
                        .subscriptionsConfigMap.find(id);
        persistent_data::EventServiceStore::getInstance()
            .subscriptionsConfigMap.erase(obj2);
        updateNoOfSubscribersCount();
        updateSubscriptionData();
    }
}

size_t EventServiceManager::getNumberOfSubscriptions()
{
    return subscriptionsMap.size();
}

std::vector<std::string> EventServiceManager::getAllIDs()
{
    std::vector<std::string> idList;
    for (const auto& it : subscriptionsMap)
    {
        idList.emplace_back(it.first);
    }
    return idList;
}

bool EventServiceManager::isDestinationExist(const std::string& destUrl)
{
    for (const auto& it : subscriptionsMap)
    {
        std::shared_ptr<Subscription> entry = it.second;
        if (entry->destinationUrl == destUrl)
        {
            BMCWEB_LOG_ERROR << "Destination exist already" << destUrl;
            return true;
        }
    }
    return false;
}

void EventServiceManager::sendTestEventLog()
{
    for (const auto& it : this->subscriptionsMap)
    {
        std::shared_ptr<Subscription> entry = it.second;
        entry->sendTestEventLog();
    }
}

void EventServiceManager::sendEvent(const nlohmann::json& eventMessageIn,
                const std::string& origin, const std::string& resType)
{
    nlohmann::json eventRecord = nlohmann::json::array();
    nlohmann::json eventMessage = eventMessageIn;
    // MemberId is 0 : since we are sending one event record.
    uint64_t memberId = 0;

    nlohmann::json event = {
        {"EventId", eventId},
        {"MemberId", memberId},
        {"EventTimestamp", crow::utility::getDateTimeOffsetNow().first},
        {"OriginOfCondition", origin}};
    for (nlohmann::json::iterator it = event.begin(); it != event.end();
            ++it)
    {
        eventMessage[it.key()] = it.value();
    }
    eventRecord.push_back(eventMessage);

    for (const auto& it : this->subscriptionsMap)
    {
        std::shared_ptr<Subscription> entry = it.second;
        bool isSubscribed = false;
        // Search the resourceTypes list for the subscription.
        // If resourceTypes list is empty, don't filter events
        // send everything.
        if (entry->resourceTypes.size())
        {
            for (const auto& resource : entry->resourceTypes)
            {
                if (resType == resource)
                {
                    BMCWEB_LOG_INFO << "ResourceType " << resource
                                    << " found in the subscribed list";
                    isSubscribed = true;
                    break;
                }
            }
        }
        else // resourceTypes list is empty.
        {
            isSubscribed = true;
        }
        if (isSubscribed)
        {
            nlohmann::json msgJson = {
                {"@odata.type", "#Event.v1_4_0.Event"},
                {"Name", "Event Log"},
                {"Id", eventId},
                {"Events", eventRecord}};
            entry->sendEvent(msgJson.dump(
                2, ' ', true, nlohmann::json::error_handler_t::replace));
            eventId++; // increament the eventId
        }
        else
        {
            BMCWEB_LOG_INFO << "Not subscribed to this resource";
        }
    }
}

void EventServiceManager::sendBroadcastMsg(const std::string& broadcastMsg)
{
    for (const auto& it : this->subscriptionsMap)
    {
        std::shared_ptr<Subscription> entry = it.second;
        nlohmann::json msgJson = {
            {"Timestamp", crow::utility::getDateTimeOffsetNow().first},
            {"OriginOfCondition", "/ibm/v1/HMC/BroadcastService"},
            {"Name", "Broadcast Message"},
            {"Message", broadcastMsg}};
        entry->sendEvent(msgJson.dump(
            2, ' ', true, nlohmann::json::error_handler_t::replace));
    }
}


#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
void EventServiceManager::cacheLastEventTimestamp()
{
    lastEventTStr.clear();
    std::ifstream logStream(redfishEventLogFile);
    if (!logStream.good())
    {
        BMCWEB_LOG_ERROR << " Redfish log file open failed \n";
        return;
    }
    std::string logEntry;
    while (std::getline(logStream, logEntry))
    {
        size_t space = logEntry.find_first_of(' ');
        if (space == std::string::npos)
        {
            // Shouldn't enter here but lets skip it.
            BMCWEB_LOG_DEBUG << "Invalid log entry found.";
            continue;
        }
        lastEventTStr = logEntry.substr(0, space);
    }
    BMCWEB_LOG_DEBUG << "Last Event time stamp set: " << lastEventTStr;
}

void EventServiceManager::readEventLogsFromFile()
{
    if (!serviceEnabled || !noOfEventLogSubscribers)
    {
        BMCWEB_LOG_DEBUG << "EventService disabled or no Subscriptions.";
        return;
    }
    std::ifstream logStream(redfishEventLogFile);
    if (!logStream.good())
    {
        BMCWEB_LOG_ERROR << " Redfish log file open failed";
        return;
    }

    std::vector<EventLogObjectsType> eventRecords;

    bool startLogCollection = false;
    bool firstEntry = true;

    std::string logEntry;
    while (std::getline(logStream, logEntry))
    {
        if (!startLogCollection && !lastEventTStr.empty())
        {
            if (boost::starts_with(logEntry, lastEventTStr))
            {
                startLogCollection = true;
            }
            continue;
        }

        std::string idStr;
        if (!event_log::getUniqueEntryID(logEntry, idStr, firstEntry))
        {
            continue;
        }
        firstEntry = false;

        std::string timestamp;
        std::string messageID;
        std::vector<std::string> messageArgs;
        if (event_log::getEventLogParams(logEntry, timestamp, messageID,
                                            messageArgs) != 0)
        {
            BMCWEB_LOG_DEBUG << "Read eventLog entry params failed";
            continue;
        }

        std::string registryName;
        std::string messageKey;
        event_log::getRegistryAndMessageKey(messageID, registryName,
                                            messageKey);
        if (registryName.empty() || messageKey.empty())
        {
            continue;
        }

        lastEventTStr = timestamp;
        eventRecords.emplace_back(idStr, timestamp, messageID, registryName,
                                    messageKey, messageArgs);
    }

    for (const auto& it : this->subscriptionsMap)
    {
        std::shared_ptr<Subscription> entry = it.second;
        if (entry->eventFormatType == "Event")
        {
            entry->filterAndSendEventLogs(eventRecords);
        }
    }
}

void EventServiceManager::watchRedfishEventLogFile()
{
    if (!inotifyConn)
    {
        return;
    }

    static std::array<char, 1024> readBuffer;

    inotifyConn->async_read_some(
        boost::asio::buffer(readBuffer),
        [&](const boost::system::error_code& ec,
            const std::size_t& bytesTransferred) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Callback Error: " << ec.message();
                return;
            }
            std::size_t index = 0;
            while ((index + iEventSize) <= bytesTransferred)
            {
                struct inotify_event event;
                std::memcpy(&event, &readBuffer[index], iEventSize);
                if (event.wd == dirWatchDesc)
                {
                    if ((event.len == 0) ||
                        (index + iEventSize + event.len > bytesTransferred))
                    {
                        index += (iEventSize + event.len);
                        continue;
                    }

                    std::string fileName(&readBuffer[index + iEventSize],
                                            event.len);
                    if (std::strcmp(fileName.c_str(), "redfish") != 0)
                    {
                        index += (iEventSize + event.len);
                        continue;
                    }

                    BMCWEB_LOG_DEBUG
                        << "Redfish log file created/deleted. event.name: "
                        << fileName;
                    if (event.mask == IN_CREATE)
                    {
                        if (fileWatchDesc != -1)
                        {
                            BMCWEB_LOG_DEBUG
                                << "Remove and Add inotify watcher on "
                                    "redfish event log file";
                            // Remove existing inotify watcher and add
                            // with new redfish event log file.
                            inotify_rm_watch(inotifyFd, fileWatchDesc);
                            fileWatchDesc = -1;
                        }

                        fileWatchDesc = inotify_add_watch(
                            inotifyFd, redfishEventLogFile, IN_MODIFY);
                        if (fileWatchDesc == -1)
                        {
                            BMCWEB_LOG_ERROR
                                << "inotify_add_watch failed for "
                                    "redfish log file.";
                            return;
                        }

                        EventServiceManager::getInstance()
                            .cacheLastEventTimestamp();
                        EventServiceManager::getInstance()
                            .readEventLogsFromFile();
                    }
                    else if ((event.mask == IN_DELETE) ||
                                (event.mask == IN_MOVED_TO))
                    {
                        if (fileWatchDesc != -1)
                        {
                            inotify_rm_watch(inotifyFd, fileWatchDesc);
                            fileWatchDesc = -1;
                        }
                    }
                }
                else if (event.wd == fileWatchDesc)
                {
                    if (event.mask == IN_MODIFY)
                    {
                        EventServiceManager::getInstance()
                            .readEventLogsFromFile();
                    }
                }
                index += (iEventSize + event.len);
            }

            watchRedfishEventLogFile();
        });
}

int EventServiceManager::startEventLogMonitor(boost::asio::io_context& ioc)
{
    inotifyConn.emplace(ioc);
    inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd == -1)
    {
        BMCWEB_LOG_ERROR << "inotify_init1 failed.";
        return -1;
    }

    // Add watch on directory to handle redfish event log file
    // create/delete.
    dirWatchDesc = inotify_add_watch(inotifyFd, redfishEventLogDir,
                                        IN_CREATE | IN_MOVED_TO | IN_DELETE);
    if (dirWatchDesc == -1)
    {
        BMCWEB_LOG_ERROR
            << "inotify_add_watch failed for event log directory.";
        return -1;
    }

    // Watch redfish event log file for modifications.
    fileWatchDesc =
        inotify_add_watch(inotifyFd, redfishEventLogFile, IN_MODIFY);
    if (fileWatchDesc == -1)
    {
        BMCWEB_LOG_ERROR
            << "inotify_add_watch failed for redfish log file.";
        // Don't return error if file not exist.
        // Watch on directory will handle create/delete of file.
    }

    // monitor redfish event log file
    inotifyConn->assign(inotifyFd);
    watchRedfishEventLogFile();

    return 0;
}
#endif

void EventServiceManager::getMetricReading(const std::string& service,
                        const std::string& objPath, const std::string& intf)
{
    std::size_t found = objPath.find_last_of('/');
    if (found == std::string::npos)
    {
        BMCWEB_LOG_DEBUG << "Invalid objPath received";
        return;
    }

    std::string idStr = objPath.substr(found + 1);
    if (idStr.empty())
    {
        BMCWEB_LOG_DEBUG << "Invalid ID in objPath";
        return;
    }

    crow::connections::systemBus->async_method_call(
        [idStr{std::move(idStr)}](
            const boost::system::error_code ec,
            boost::container::flat_map<
                std::string, std::variant<int32_t, ReadingsObjType>>&
                resp) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "D-Bus call failed to GetAll metric readings.";
                return;
            }

            const int32_t* timestampPtr =
                std::get_if<int32_t>(&resp["Timestamp"]);
            if (!timestampPtr)
            {
                BMCWEB_LOG_DEBUG << "Failed to Get timestamp.";
                return;
            }

            ReadingsObjType* readingsPtr =
                std::get_if<ReadingsObjType>(&resp["Readings"]);
            if (!readingsPtr)
            {
                BMCWEB_LOG_DEBUG << "Failed to Get Readings property.";
                return;
            }

            if (!readingsPtr->size())
            {
                BMCWEB_LOG_DEBUG << "No metrics report to be transferred";
                return;
            }

            for (const auto& it :
                    EventServiceManager::getInstance().subscriptionsMap)
            {
                std::shared_ptr<Subscription> entry = it.second;
                if (entry->eventFormatType == metricReportFormatType)
                {
                    entry->filterAndSendReports(
                        idStr, crow::utility::getDateTime(*timestampPtr),
                        *readingsPtr);
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        intf);
}

void EventServiceManager::unregisterMetricReportSignal()
{
    if (matchTelemetryMonitor)
    {
        BMCWEB_LOG_DEBUG << "Metrics report signal - Unregister";
        matchTelemetryMonitor.reset();
        matchTelemetryMonitor = nullptr;
    }
}

void EventServiceManager::registerMetricReportSignal()
{
    if (!serviceEnabled || matchTelemetryMonitor)
    {
        BMCWEB_LOG_DEBUG << "Not registering metric report signal.";
        return;
    }

    BMCWEB_LOG_DEBUG << "Metrics report signal - Register";
    std::string matchStr(
        "type='signal',member='ReportUpdate', "
        "interface='xyz.openbmc_project.MonitoringService.Report'");

    matchTelemetryMonitor = std::make_shared<sdbusplus::bus::match::match>(
        *crow::connections::systemBus, matchStr,
        [this](sdbusplus::message::message& msg) {
            if (msg.is_method_error())
            {
                BMCWEB_LOG_ERROR << "TelemetryMonitor Signal error";
                return;
            }

            std::string service = msg.get_sender();
            std::string objPath = msg.get_path();
            std::string intf = msg.get_interface();
            getMetricReading(service, objPath, intf);
        });
}

bool EventServiceManager::validateAndSplitUrl(const std::string& destUrl, std::string& urlProto,
                            std::string& host, std::string& port,
                            std::string& path)
{
    // Validate URL using regex expression
    // Format: <protocol>://<host>:<port>/<path>
    // protocol: http/https
    const std::regex urlRegex(
        "(http|https)://([^/\\x20\\x3f\\x23\\x3a]+):?([0-9]*)(/"
        "([^\\x20\\x23\\x3f]*\\x3f?([^\\x20\\x23\\x3f])*)?)");
    std::cmatch match;
    if (!std::regex_match(destUrl.c_str(), match, urlRegex))
    {
        BMCWEB_LOG_INFO << "Dest. url did not match ";
        return false;
    }

    urlProto = std::string(match[1].first, match[1].second);
    if (urlProto == "http")
    {
#ifndef BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING
        return false;
#endif
    }

    host = std::string(match[2].first, match[2].second);
    port = std::string(match[3].first, match[3].second);
    path = std::string(match[4].first, match[4].second);
    if (port.empty())
    {
        if (urlProto == "http")
        {
            port = "80";
        }
        else
        {
            port = "443";
        }
    }
    if (path.empty())
    {
        path = "/";
    }
    return true;
}

}
