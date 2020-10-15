/*
// Copyright (c) 2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#include "node.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"

#include <sys/inotify.h>

#include <boost/asio/io_context.hpp>
#include <boost/container/flat_map.hpp>
#include <error_messages.hpp>
#include <http_client.hpp>
#include <random.hpp>
#include <server_sent_events.hpp>
#include <utils/json_utils.hpp>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <memory>
#include <variant>

namespace redfish
{

using ReadingsObjType =
    std::vector<std::tuple<std::string, std::string, double, int32_t>>;
using EventServiceConfig = std::tuple<bool, uint32_t, uint32_t>;

static constexpr const char* eventFormatType = "Event";
static constexpr const char* metricReportFormatType = "MetricReport";

static constexpr const char* eventServiceFile =
    "/var/lib/bmcweb/eventservice_config.json";

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
static std::optional<boost::asio::posix::stream_descriptor> inotifyConn;
static constexpr const char* redfishEventLogDir = "/var/log";
static constexpr const char* redfishEventLogFile = "/var/log/redfish";
static constexpr const size_t iEventSize = sizeof(inotify_event);
static int inotifyFd = -1;
static int dirWatchDesc = -1;
static int fileWatchDesc = -1;

// <ID, timestamp, RedfishLogId, registryPrefix, MessageId, MessageArgs>
using EventLogObjectsType =
    std::tuple<std::string, std::string, std::string, std::string, std::string,
               std::vector<std::string>>;

namespace message_registries
{
static const Message*
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

static const Message* formatMessage(const std::string_view& messageID)
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
    if (std::string(base::header.registryPrefix) == registryName)
    {
        return getMsgFromRegistry(
            messageKey, boost::beast::span<const MessageEntry>(base::registry));
    }
    if (std::string(openbmc::header.registryPrefix) == registryName)
    {
        return getMsgFromRegistry(
            messageKey,
            boost::beast::span<const MessageEntry>(openbmc::registry));
    }
    return nullptr;
}
} // namespace message_registries

namespace event_log
{
inline bool getUniqueEntryID(const std::string& logEntry, std::string& entryID,
                             const bool firstEntry = true)
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

inline int getEventLogParams(const std::string& logEntry,
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

inline void getRegistryAndMessageKey(const std::string& messageID,
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

inline int formatEventLogEntry(const std::string& logEntryID,
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

} // namespace event_log
#endif

inline bool isFilterQuerySpecialChar(char c)
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

inline bool
    readSSEQueryParams(std::string sseFilter, std::string& formatType,
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

class Subscription
{
  public:
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

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    Subscription(const std::string& inHost, const std::string& inPort,
                 const std::string& inPath, const std::string& inUriProto) :
        eventSeqNum(1),
        host(inHost), port(inPort), path(inPath), uriProto(inUriProto)
    {
        conn = std::make_shared<crow::HttpClient>(
            crow::connections::systemBus->get_io_context(), id, host, port,
            path);
    }

    Subscription(const std::shared_ptr<boost::beast::tcp_stream>& adaptor) :
        eventSeqNum(1)
    {
        sseConn = std::make_shared<crow::ServerSentEvents>(adaptor);
    }

    ~Subscription() = default;

    void sendEvent(const std::string& msg)
    {
        if (conn != nullptr)
        {
            std::vector<std::pair<std::string, std::string>> reqHeaders;
            for (const auto& header : httpHeaders)
            {
                for (const auto& item : header.items())
                {
                    std::string key = item.key();
                    std::string val = item.value();
                    reqHeaders.emplace_back(std::pair(key, val));
                }
            }
            conn->setHeaders(reqHeaders);
            conn->sendData(msg);
            this->eventSeqNum++;
        }

        if (sseConn != nullptr)
        {
            sseConn->sendData(eventSeqNum, msg);
        }
    }

    void sendTestEventLog()
    {
        nlohmann::json logEntryArray;
        logEntryArray.push_back({});
        nlohmann::json& logEntryJson = logEntryArray.back();

        logEntryJson = {{"EventId", "TestID"},
                        {"EventType", "Event"},
                        {"Severity", "OK"},
                        {"Message", "Generated test event"},
                        {"MessageId", "OpenBMC.0.1.TestEventLog"},
                        {"MessageArgs", nlohmann::json::array()},
                        {"EventTimestamp", crow::utility::dateTimeNow()},
                        {"Context", customText}};

        nlohmann::json msg = {{"@odata.type", "#Event.v1_4_0.Event"},
                              {"Id", std::to_string(eventSeqNum)},
                              {"Name", "Event Log"},
                              {"Events", logEntryArray}};

        this->sendEvent(msg.dump());
    }

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    void filterAndSendEventLogs(
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

        this->sendEvent(msg.dump());
    }
#endif

    void filterAndSendReports(const std::string& id2,
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

        this->sendEvent(msg.dump());
    }

    void updateRetryConfig(const uint32_t retryAttempts,
                           const uint32_t retryTimeoutInterval)
    {
        if (conn != nullptr)
        {
            conn->setRetryConfig(retryAttempts, retryTimeoutInterval);
        }
    }

    void updateRetryPolicy()
    {
        if (conn != nullptr)
        {
            conn->setRetryPolicy(retryPolicy);
        }
    }

    uint64_t getEventSeqNum()
    {
        return eventSeqNum;
    }

  private:
    uint64_t eventSeqNum;
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<crow::HttpClient> conn = nullptr;
    std::shared_ptr<crow::ServerSentEvents> sseConn = nullptr;
};

static constexpr const bool defaultEnabledState = true;
static constexpr const uint32_t defaultRetryAttempts = 3;
static constexpr const uint32_t defaultRetryInterval = 30;
static constexpr const char* defaulEventFormatType = "Event";
static constexpr const char* defaulSubscriptionType = "RedfishEvent";
static constexpr const char* defaulRetryPolicy = "TerminateAfterRetries";

class EventServiceManager
{
  private:
    bool serviceEnabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;

    EventServiceManager()
    {
        // Load config from persist store.
        initConfig();
    }

    std::string lastEventTStr;
    size_t noOfEventLogSubscribers{0};
    size_t noOfMetricReportSubscribers{0};
    std::shared_ptr<sdbusplus::bus::match::match> matchTelemetryMonitor;
    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        subscriptionsMap;

    uint64_t eventId{1};

  public:
    EventServiceManager(const EventServiceManager&) = delete;
    EventServiceManager& operator=(const EventServiceManager&) = delete;
    EventServiceManager(EventServiceManager&&) = delete;
    EventServiceManager& operator=(EventServiceManager&&) = delete;

    static EventServiceManager& getInstance()
    {
        static EventServiceManager handler;
        return handler;
    }

    void loadDefaultConfig()
    {
        serviceEnabled = defaultEnabledState;
        retryAttempts = defaultRetryAttempts;
        retryTimeoutInterval = defaultRetryInterval;
    }

    void initConfig()
    {
        std::ifstream eventConfigFile(eventServiceFile);
        if (!eventConfigFile.good())
        {
            BMCWEB_LOG_DEBUG << "EventService config not exist";
            loadDefaultConfig();
            return;
        }
        auto jsonData = nlohmann::json::parse(eventConfigFile, nullptr, false);
        if (jsonData.is_discarded())
        {
            BMCWEB_LOG_ERROR << "EventService config parse error.";
            loadDefaultConfig();
            return;
        }

        nlohmann::json jsonConfig;
        if (json_util::getValueFromJsonObject(jsonData, "Configuration",
                                              jsonConfig))
        {
            if (!json_util::getValueFromJsonObject(jsonConfig, "ServiceEnabled",
                                                   serviceEnabled))
            {
                serviceEnabled = defaultEnabledState;
            }
            if (!json_util::getValueFromJsonObject(
                    jsonConfig, "DeliveryRetryAttempts", retryAttempts))
            {
                retryAttempts = defaultRetryAttempts;
            }
            if (!json_util::getValueFromJsonObject(
                    jsonConfig, "DeliveryRetryIntervalSeconds",
                    retryTimeoutInterval))
            {
                retryTimeoutInterval = defaultRetryInterval;
            }
        }
        else
        {
            loadDefaultConfig();
        }

        nlohmann::json subscriptionsList;
        if (!json_util::getValueFromJsonObject(jsonData, "Subscriptions",
                                               subscriptionsList))
        {
            BMCWEB_LOG_DEBUG << "EventService: Subscriptions not exist.";
            return;
        }

        for (nlohmann::json& jsonObj : subscriptionsList)
        {
            std::string protocol;
            if (!json_util::getValueFromJsonObject(jsonObj, "Protocol",
                                                   protocol))
            {
                BMCWEB_LOG_DEBUG << "Invalid subscription Protocol exist.";
                continue;
            }

            std::string subscriptionType;
            if (!json_util::getValueFromJsonObject(jsonObj, "SubscriptionType",
                                                   subscriptionType))
            {
                subscriptionType = defaulSubscriptionType;
            }
            // SSE connections are initiated from client
            // and can't be re-established from server.
            if (subscriptionType == "SSE")
            {
                BMCWEB_LOG_DEBUG
                    << "The subscription type is SSE, so skipping.";
                continue;
            }

            std::string destination;
            if (!json_util::getValueFromJsonObject(jsonObj, "Destination",
                                                   destination))
            {
                BMCWEB_LOG_DEBUG << "Invalid subscription destination exist.";
                continue;
            }
            std::string host;
            std::string urlProto;
            std::string port;
            std::string path;
            bool status =
                validateAndSplitUrl(destination, urlProto, host, port, path);

            if (!status)
            {
                BMCWEB_LOG_ERROR
                    << "Failed to validate and split destination url";
                continue;
            }
            std::shared_ptr<Subscription> subValue =
                std::make_shared<Subscription>(host, port, path, urlProto);

            subValue->destinationUrl = destination;
            subValue->protocol = protocol;
            subValue->subscriptionType = subscriptionType;
            if (!json_util::getValueFromJsonObject(
                    jsonObj, "DeliveryRetryPolicy", subValue->retryPolicy))
            {
                subValue->retryPolicy = defaulRetryPolicy;
            }
            if (!json_util::getValueFromJsonObject(jsonObj, "EventFormatType",
                                                   subValue->eventFormatType))
            {
                subValue->eventFormatType = defaulEventFormatType;
            }
            json_util::getValueFromJsonObject(jsonObj, "Context",
                                              subValue->customText);
            json_util::getValueFromJsonObject(jsonObj, "MessageIds",
                                              subValue->registryMsgIds);
            json_util::getValueFromJsonObject(jsonObj, "RegistryPrefixes",
                                              subValue->registryPrefixes);
            json_util::getValueFromJsonObject(jsonObj, "ResourceTypes",
                                              subValue->resourceTypes);
            json_util::getValueFromJsonObject(jsonObj, "HttpHeaders",
                                              subValue->httpHeaders);
            json_util::getValueFromJsonObject(
                jsonObj, "MetricReportDefinitions",
                subValue->metricReportDefinitions);

            std::string id = addSubscription(subValue, false);
            if (id.empty())
            {
                BMCWEB_LOG_ERROR << "Failed to add subscription";
            }
        }
        return;
    }

    void updateSubscriptionData()
    {
        // Persist the config and subscription data.
        nlohmann::json jsonData;

        nlohmann::json& configObj = jsonData["Configuration"];
        configObj["ServiceEnabled"] = serviceEnabled;
        configObj["DeliveryRetryAttempts"] = retryAttempts;
        configObj["DeliveryRetryIntervalSeconds"] = retryTimeoutInterval;

        nlohmann::json& subListArray = jsonData["Subscriptions"];
        subListArray = nlohmann::json::array();

        for (const auto& it : subscriptionsMap)
        {
            std::shared_ptr<Subscription> subValue = it.second;
            // Don't preserve SSE connections. Its initiated from
            // client side and can't be re-established from server.
            if (subValue->subscriptionType == "SSE")
            {
                BMCWEB_LOG_DEBUG
                    << "The subscription type is SSE, so skipping.";
                continue;
            }

            nlohmann::json entry;
            entry["Context"] = subValue->customText;
            entry["DeliveryRetryPolicy"] = subValue->retryPolicy;
            entry["Destination"] = subValue->destinationUrl;
            entry["EventFormatType"] = subValue->eventFormatType;
            entry["HttpHeaders"] = subValue->httpHeaders;
            entry["MessageIds"] = subValue->registryMsgIds;
            entry["Protocol"] = subValue->protocol;
            entry["RegistryPrefixes"] = subValue->registryPrefixes;
            entry["ResourceTypes"] = subValue->resourceTypes;
            entry["SubscriptionType"] = subValue->subscriptionType;
            entry["MetricReportDefinitions"] =
                subValue->metricReportDefinitions;

            subListArray.push_back(entry);
        }

        const std::string tmpFile(std::string(eventServiceFile) + "_tmp");
        std::ofstream ofs(tmpFile, std::ios::out);
        const auto& writeData = jsonData.dump();
        ofs << writeData;
        ofs.close();

        BMCWEB_LOG_DEBUG << "EventService config updated to file.";
        if (std::rename(tmpFile.c_str(), eventServiceFile) != 0)
        {
            BMCWEB_LOG_ERROR << "Error in renaming temporary file: "
                             << tmpFile.c_str();
        }
    }

    EventServiceConfig getEventServiceConfig()
    {
        return {serviceEnabled, retryAttempts, retryTimeoutInterval};
    }

    void setEventServiceConfig(const EventServiceConfig& cfg)
    {
        bool updateConfig = false;
        bool updateRetryCfg = false;

        if (serviceEnabled != std::get<0>(cfg))
        {
            serviceEnabled = std::get<0>(cfg);
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

        if (retryAttempts != std::get<1>(cfg))
        {
            retryAttempts = std::get<1>(cfg);
            updateConfig = true;
            updateRetryCfg = true;
        }

        if (retryTimeoutInterval != std::get<2>(cfg))
        {
            retryTimeoutInterval = std::get<2>(cfg);
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

    void updateNoOfSubscribersCount()
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

    std::shared_ptr<Subscription> getSubscription(const std::string& id)
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

    std::string addSubscription(const std::shared_ptr<Subscription>& subValue,
                                const bool updateFile = true)
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
            return std::string("");
        }

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

    bool isSubscriptionExist(const std::string& id)
    {
        auto obj = subscriptionsMap.find(id);
        if (obj == subscriptionsMap.end())
        {
            return false;
        }
        return true;
    }

    void deleteSubscription(const std::string& id)
    {
        auto obj = subscriptionsMap.find(id);
        if (obj != subscriptionsMap.end())
        {
            subscriptionsMap.erase(obj);
            updateNoOfSubscribersCount();
            updateSubscriptionData();
        }
    }

    size_t getNumberOfSubscriptions()
    {
        return subscriptionsMap.size();
    }

    std::vector<std::string> getAllIDs()
    {
        std::vector<std::string> idList;
        for (const auto& it : subscriptionsMap)
        {
            idList.emplace_back(it.first);
        }
        return idList;
    }

    bool isDestinationExist(const std::string& destUrl)
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

    void sendTestEventLog()
    {
        for (const auto& it : this->subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            entry->sendTestEventLog();
        }
    }

    void sendEvent(const nlohmann::json& eventMessageIn,
                   const std::string& origin, const std::string& resType)
    {
        nlohmann::json eventRecord = nlohmann::json::array();
        nlohmann::json eventMessage = eventMessageIn;
        // MemberId is 0 : since we are sending one event record.
        uint64_t memberId = 0;

        nlohmann::json event = {
            {"EventId", eventId},
            {"MemberId", memberId},
            {"EventTimestamp", crow::utility::dateTimeNow()},
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
                entry->sendEvent(msgJson.dump());
                eventId++; // increament the eventId
            }
            else
            {
                BMCWEB_LOG_INFO << "Not subscribed to this resource";
            }
        }
    }
    void sendBroadcastMsg(const std::string& broadcastMsg)
    {
        for (const auto& it : this->subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            nlohmann::json msgJson = {
                {"Timestamp", crow::utility::dateTimeNow()},
                {"OriginOfCondition", "/ibm/v1/HMC/BroadcastService"},
                {"Name", "Broadcast Message"},
                {"Message", broadcastMsg}};
            entry->sendEvent(msgJson.dump());
        }
    }

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    void cacheLastEventTimestamp()
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

    void readEventLogsFromFile()
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

    static void watchRedfishEventLogFile()
    {
        if (inotifyConn)
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

    static int startEventLogMonitor(boost::asio::io_context& ioc)
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

    void getMetricReading(const std::string& service,
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

    void unregisterMetricReportSignal()
    {
        if (matchTelemetryMonitor)
        {
            BMCWEB_LOG_DEBUG << "Metrics report signal - Unregister";
            matchTelemetryMonitor.reset();
            matchTelemetryMonitor = nullptr;
        }
    }

    void registerMetricReportSignal()
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

    bool validateAndSplitUrl(const std::string& destUrl, std::string& urlProto,
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
};

} // namespace redfish
