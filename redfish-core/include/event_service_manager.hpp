/*
Copyright (c) 2020 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma once
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "event_matches_filter.hpp"
#include "event_service_store.hpp"
#include "filter_expr_executor.hpp"
#include "generated/enums/event.hpp"
#include "generated/enums/log_entry.hpp"
#include "http_client.hpp"
#include "metric_report.hpp"
#include "ossl_random.hpp"
#include "persistent_data.hpp"
#include "registries.hpp"
#include "registries_selector.hpp"
#include "str_utility.hpp"
#include "utility.hpp"
#include "utils/json_utils.hpp"
#include "utils/time_utils.hpp"

#include <sys/inotify.h>

#include <boost/asio/io_context.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url_view_base.hpp>
#include <sdbusplus/bus/match.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <memory>
#include <ranges>
#include <span>
#include <string>

namespace redfish
{

static constexpr const char* eventFormatType = "Event";
static constexpr const char* metricReportFormatType = "MetricReport";

static constexpr const char* subscriptionTypeSSE = "SSE";
static constexpr const char* eventServiceFile =
    "/var/lib/bmcweb/eventservice_config.json";

static constexpr const uint8_t maxNoOfSubscriptions = 20;
static constexpr const uint8_t maxNoOfSSESubscriptions = 10;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::optional<boost::asio::posix::stream_descriptor> inotifyConn;
static constexpr const char* redfishEventLogDir = "/var/log";
static constexpr const char* redfishEventLogFile = "/var/log/redfish";
static constexpr const size_t iEventSize = sizeof(inotify_event);

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int inotifyFd = -1;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int dirWatchDesc = -1;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int fileWatchDesc = -1;
struct EventLogObjectsType
{
    std::string id;
    std::string timestamp;
    std::string messageId;
    std::vector<std::string> messageArgs;
};

namespace registries
{
static const Message*
    getMsgFromRegistry(const std::string& messageKey,
                       const std::span<const MessageEntry>& registry)
{
    std::span<const MessageEntry>::iterator messageIt = std::ranges::find_if(
        registry, [&messageKey](const MessageEntry& messageEntry) {
            return messageKey == messageEntry.first;
        });
    if (messageIt != registry.end())
    {
        return &messageIt->second;
    }

    return nullptr;
}

static const Message* formatMessage(std::string_view messageID)
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
    return getMsgFromRegistry(messageKey, getRegistryFromPrefix(registryName));
}
} // namespace registries

namespace event_log
{
inline bool getUniqueEntryID(const std::string& logEntry, std::string& entryID)
{
    static time_t prevTs = 0;
    static int index = 0;

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
        BMCWEB_LOG_ERROR("EventLog Params: could not find first space: {}",
                         logEntry);
        return -EINVAL;
    }
    timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(' ', space);
    if (entryStart == std::string::npos)
    {
        BMCWEB_LOG_ERROR("EventLog Params: could not find log contents: {}",
                         logEntry);
        return -EINVAL;
    }
    std::string_view entry(logEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> logEntryFields;
    bmcweb::split(logEntryFields, entry, ',');
    // We need at least a MessageId to be valid
    if (logEntryFields.empty())
    {
        BMCWEB_LOG_ERROR("EventLog Params: could not find entry fields: {}",
                         logEntry);
        return -EINVAL;
    }
    messageID = logEntryFields[0];

    // Get the MessageArgs from the log if there are any
    if (logEntryFields.size() > 1)
    {
        const std::string& messageArgsStart = logEntryFields[1];
        // If the first string is empty, assume there are no MessageArgs
        if (!messageArgsStart.empty())
        {
            messageArgs.assign(logEntryFields.begin() + 1,
                               logEntryFields.end());
        }
    }

    return 0;
}

inline int formatEventLogEntry(
    const std::string& logEntryID, const std::string& messageID,
    const std::span<std::string_view> messageArgs, std::string timestamp,
    const std::string& customText, nlohmann::json::object_t& logEntryJson)
{
    // Get the Message from the MessageRegistry
    const registries::Message* message = registries::formatMessage(messageID);

    if (message == nullptr)
    {
        return -1;
    }

    std::string msg =
        redfish::registries::fillMessageArgs(messageArgs, message->message);
    if (msg.empty())
    {
        return -1;
    }

    // Get the Created time from the timestamp. The log timestamp is in
    // RFC3339 format which matches the Redfish format except for the
    // fractional seconds between the '.' and the '+', so just remove them.
    std::size_t dot = timestamp.find_first_of('.');
    std::size_t plus = timestamp.find_first_of('+', dot);
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    // Fill in the log entry with the gathered data
    logEntryJson["EventId"] = logEntryID;

    logEntryJson["Severity"] = message->messageSeverity;
    logEntryJson["Message"] = std::move(msg);
    logEntryJson["MessageId"] = messageID;
    logEntryJson["MessageArgs"] = messageArgs;
    logEntryJson["EventTimestamp"] = std::move(timestamp);
    logEntryJson["Context"] = customText;
    return 0;
}

} // namespace event_log

class Subscription
{
  public:
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    Subscription(const persistent_data::UserSubscription& userSubIn,
                 const boost::urls::url_view_base& url,
                 boost::asio::io_context& ioc) :
        userSub(userSubIn), policy(std::make_shared<crow::ConnectionPolicy>())
    {
        userSub.destinationUrl = url;
        client.emplace(ioc, policy);
        // Subscription constructor
        policy->invalidResp = retryRespHandler;
    }

    explicit Subscription(crow::sse_socket::Connection& connIn) :
        sseConn(&connIn)
    {}

    ~Subscription() = default;

    bool sendEventToSubscriber(std::string&& msg)
    {
        persistent_data::EventServiceConfig eventServiceConfig =
            persistent_data::EventServiceStore::getInstance()
                .getEventServiceConfig();
        if (!eventServiceConfig.enabled)
        {
            return false;
        }

        if (client)
        {
            client->sendData(std::move(msg), userSub.destinationUrl,
                             static_cast<ensuressl::VerifyCertificate>(
                                 userSub.verifyCertificate),
                             userSub.httpHeaders,
                             boost::beast::http::verb::post);
            return true;
        }

        if (sseConn != nullptr)
        {
            eventSeqNum++;
            sseConn->sendSseEvent(std::to_string(eventSeqNum), msg);
        }
        return true;
    }

    bool sendTestEventLog()
    {
        nlohmann::json::array_t logEntryArray;
        nlohmann::json& logEntryJson = logEntryArray.emplace_back();

        logEntryJson["EventId"] = "TestID";
        logEntryJson["Severity"] = log_entry::EventSeverity::OK;
        logEntryJson["Message"] = "Generated test event";
        logEntryJson["MessageId"] = "OpenBMC.0.2.TestEventLog";
        // MemberId is 0 : since we are sending one event record.
        logEntryJson["MemberId"] = "0";
        logEntryJson["MessageArgs"] = nlohmann::json::array();
        logEntryJson["EventTimestamp"] =
            redfish::time_utils::getDateTimeOffsetNow().first;
        logEntryJson["Context"] = userSub.customText;

        nlohmann::json msg;
        msg["@odata.type"] = "#Event.v1_4_0.Event";
        msg["Id"] = std::to_string(eventSeqNum);
        msg["Name"] = "Event Log";
        msg["Events"] = logEntryArray;

        std::string strMsg =
            msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
        return sendEventToSubscriber(std::move(strMsg));
    }

    void filterAndSendEventLogs(
        const std::vector<EventLogObjectsType>& eventRecords)
    {
        nlohmann::json::array_t logEntryArray;
        for (const EventLogObjectsType& logEntry : eventRecords)
        {
            std::vector<std::string_view> messageArgsView(
                logEntry.messageArgs.begin(), logEntry.messageArgs.end());

            nlohmann::json::object_t bmcLogEntry;
            if (event_log::formatEventLogEntry(
                    logEntry.id, logEntry.messageId, messageArgsView,
                    logEntry.timestamp, userSub.customText, bmcLogEntry) != 0)
            {
                BMCWEB_LOG_DEBUG("Read eventLog entry failed");
                continue;
            }

            if (!eventMatchesFilter(userSub, bmcLogEntry, ""))
            {
                BMCWEB_LOG_DEBUG("Event {} did not match the filter",
                                 nlohmann::json(bmcLogEntry).dump());
                continue;
            }

            if (filter)
            {
                if (!memberMatches(bmcLogEntry, *filter))
                {
                    BMCWEB_LOG_DEBUG("Filter didn't match");
                    continue;
                }
            }

            logEntryArray.emplace_back(std::move(bmcLogEntry));
        }

        if (logEntryArray.empty())
        {
            BMCWEB_LOG_DEBUG("No log entries available to be transferred.");
            return;
        }

        nlohmann::json msg;
        msg["@odata.type"] = "#Event.v1_4_0.Event";
        msg["Id"] = std::to_string(eventSeqNum);
        msg["Name"] = "Event Log";
        msg["Events"] = std::move(logEntryArray);
        std::string strMsg =
            msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
        sendEventToSubscriber(std::move(strMsg));
        eventSeqNum++;
    }

    void filterAndSendReports(const std::string& reportId,
                              const telemetry::TimestampReadings& var)
    {
        boost::urls::url mrdUri = boost::urls::format(
            "/redfish/v1/TelemetryService/MetricReportDefinitions/{}",
            reportId);

        // Empty list means no filter. Send everything.
        if (!userSub.metricReportDefinitions.empty())
        {
            if (std::ranges::find(userSub.metricReportDefinitions,
                                  mrdUri.buffer()) ==
                userSub.metricReportDefinitions.end())
            {
                return;
            }
        }

        nlohmann::json msg;
        if (!telemetry::fillReport(msg, reportId, var))
        {
            BMCWEB_LOG_ERROR("Failed to fill the MetricReport for DBus "
                             "Report with id {}",
                             reportId);
            return;
        }

        // Context is set by user during Event subscription and it must be
        // set for MetricReport response.
        if (!userSub.customText.empty())
        {
            msg["Context"] = userSub.customText;
        }

        std::string strMsg =
            msg.dump(2, ' ', true, nlohmann::json::error_handler_t::replace);
        sendEventToSubscriber(std::move(strMsg));
    }

    void updateRetryConfig(uint32_t retryAttempts,
                           uint32_t retryTimeoutInterval)
    {
        if (policy == nullptr)
        {
            BMCWEB_LOG_DEBUG("Retry policy was nullptr, ignoring set");
            return;
        }
        policy->maxRetryAttempts = retryAttempts;
        policy->retryIntervalSecs = std::chrono::seconds(retryTimeoutInterval);
    }

    uint64_t getEventSeqNum() const
    {
        return eventSeqNum;
    }

    void setSubscriptionId(const std::string& id2)
    {
        BMCWEB_LOG_DEBUG("Subscription ID: {}", id2);
        subId = id2;
    }

    std::string getSubscriptionId()
    {
        return subId;
    }

    bool matchSseId(const crow::sse_socket::Connection& thisConn)
    {
        return &thisConn == sseConn;
    }

    // Check used to indicate what response codes are valid as part of our retry
    // policy.  2XX is considered acceptable
    static boost::system::error_code retryRespHandler(unsigned int respCode)
    {
        BMCWEB_LOG_DEBUG(
            "Checking response code validity for SubscriptionEvent");
        if ((respCode < 200) || (respCode >= 300))
        {
            return boost::system::errc::make_error_code(
                boost::system::errc::result_out_of_range);
        }

        // Return 0 if the response code is valid
        return boost::system::errc::make_error_code(
            boost::system::errc::success);
    }

    persistent_data::UserSubscription userSub;

  private:
    std::string subId;
    uint64_t eventSeqNum = 1;
    boost::urls::url host;
    std::shared_ptr<crow::ConnectionPolicy> policy;
    crow::sse_socket::Connection* sseConn = nullptr;

    std::optional<crow::HttpClient> client;

  public:
    std::optional<filter_ast::LogicalAnd> filter;
};

class EventServiceManager
{
  private:
    bool serviceEnabled = false;
    uint32_t retryAttempts = 0;
    uint32_t retryTimeoutInterval = 0;

    std::streampos redfishLogFilePosition{0};
    size_t noOfEventLogSubscribers{0};
    size_t noOfMetricReportSubscribers{0};
    std::shared_ptr<sdbusplus::bus::match_t> matchTelemetryMonitor;
    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        subscriptionsMap;

    uint64_t eventId{1};

    struct Event
    {
        std::string id;
        nlohmann::json message;
    };

    constexpr static size_t maxMessages = 200;
    boost::circular_buffer<Event> messages{maxMessages};

    boost::asio::io_context& ioc;

  public:
    EventServiceManager(const EventServiceManager&) = delete;
    EventServiceManager& operator=(const EventServiceManager&) = delete;
    EventServiceManager(EventServiceManager&&) = delete;
    EventServiceManager& operator=(EventServiceManager&&) = delete;
    ~EventServiceManager() = default;

    explicit EventServiceManager(boost::asio::io_context& iocIn) : ioc(iocIn)
    {
        // Load config from persist store.
        initConfig();
    }

    static EventServiceManager&
        getInstance(boost::asio::io_context* ioc = nullptr)
    {
        static EventServiceManager handler(*ioc);
        return handler;
    }

    void initConfig()
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
            const persistent_data::UserSubscription& newSub = it.second;

            boost::system::result<boost::urls::url> url =
                boost::urls::parse_absolute_uri(newSub.destinationUrl);

            if (!url)
            {
                BMCWEB_LOG_ERROR(
                    "Failed to validate and split destination url");
                continue;
            }
            std::shared_ptr<Subscription> subValue =
                std::make_shared<Subscription>(newSub, *url, ioc);

            subscriptionsMap.insert(std::pair(subValue->userSub.id, subValue));

            updateNoOfSubscribersCount();

            if constexpr (!BMCWEB_REDFISH_DBUS_LOG)
            {
                cacheRedfishLogFile();
            }

            // Update retry configuration.
            subValue->updateRetryConfig(retryAttempts, retryTimeoutInterval);
        }
    }

    static void loadOldBehavior()
    {
        std::ifstream eventConfigFile(eventServiceFile);
        if (!eventConfigFile.good())
        {
            BMCWEB_LOG_DEBUG("Old eventService config not exist");
            return;
        }
        auto jsonData = nlohmann::json::parse(eventConfigFile, nullptr, false);
        if (jsonData.is_discarded())
        {
            BMCWEB_LOG_ERROR("Old eventService config parse error.");
            return;
        }

        const nlohmann::json::object_t* obj =
            jsonData.get_ptr<const nlohmann::json::object_t*>();
        for (const auto& item : *obj)
        {
            if (item.first == "Configuration")
            {
                persistent_data::EventServiceStore::getInstance()
                    .getEventServiceConfig()
                    .fromJson(item.second);
            }
            else if (item.first == "Subscriptions")
            {
                for (const auto& elem : item.second)
                {
                    std::optional<persistent_data::UserSubscription>
                        newSubscription =
                            persistent_data::UserSubscription::fromJson(elem,
                                                                        true);
                    if (!newSubscription)
                    {
                        BMCWEB_LOG_ERROR("Problem reading subscription "
                                         "from old persistent store");
                        continue;
                    }
                    persistent_data::UserSubscription& newSub =
                        *newSubscription;

                    std::uniform_int_distribution<uint32_t> dist(0);
                    bmcweb::OpenSSLGenerator gen;

                    std::string id;

                    int retry = 3;
                    while (retry != 0)
                    {
                        id = std::to_string(dist(gen));
                        if (gen.error())
                        {
                            retry = 0;
                            break;
                        }
                        newSub.id = id;
                        auto inserted =
                            persistent_data::EventServiceStore::getInstance()
                                .subscriptionsConfigMap.insert(
                                    std::pair(id, newSub));
                        if (inserted.second)
                        {
                            break;
                        }
                        --retry;
                    }

                    if (retry <= 0)
                    {
                        BMCWEB_LOG_ERROR(
                            "Failed to generate random number from old "
                            "persistent store");
                        continue;
                    }
                }
            }

            persistent_data::getConfig().writeData();
            std::error_code ec;
            std::filesystem::remove(eventServiceFile, ec);
            if (ec)
            {
                BMCWEB_LOG_DEBUG(
                    "Failed to remove old event service file.  Ignoring");
            }
            else
            {
                BMCWEB_LOG_DEBUG("Remove old eventservice config");
            }
        }
    }

    void updateSubscriptionData() const
    {
        persistent_data::EventServiceStore::getInstance()
            .eventServiceConfig.enabled = serviceEnabled;
        persistent_data::EventServiceStore::getInstance()
            .eventServiceConfig.retryAttempts = retryAttempts;
        persistent_data::EventServiceStore::getInstance()
            .eventServiceConfig.retryTimeoutInterval = retryTimeoutInterval;

        persistent_data::getConfig().writeData();
    }

    void setEventServiceConfig(const persistent_data::EventServiceConfig& cfg)
    {
        bool updateConfig = false;
        bool updateRetryCfg = false;

        if (serviceEnabled != cfg.enabled)
        {
            serviceEnabled = cfg.enabled;
            if (serviceEnabled && noOfMetricReportSubscribers != 0U)
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
                Subscription& entry = *it.second;
                entry.updateRetryConfig(retryAttempts, retryTimeoutInterval);
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
            if (entry->userSub.eventFormatType == eventFormatType)
            {
                eventLogSubCount++;
            }
            else if (entry->userSub.eventFormatType == metricReportFormatType)
            {
                metricReportSubCount++;
            }
        }

        noOfEventLogSubscribers = eventLogSubCount;
        if (noOfMetricReportSubscribers != metricReportSubCount)
        {
            noOfMetricReportSubscribers = metricReportSubCount;
            if (noOfMetricReportSubscribers != 0U)
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
            BMCWEB_LOG_ERROR("No subscription exist with ID:{}", id);
            return nullptr;
        }
        std::shared_ptr<Subscription> subValue = obj->second;
        return subValue;
    }

    std::string
        addSubscriptionInternal(const std::shared_ptr<Subscription>& subValue)
    {
        std::uniform_int_distribution<uint32_t> dist(0);
        bmcweb::OpenSSLGenerator gen;

        std::string id;

        int retry = 3;
        while (retry != 0)
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
            BMCWEB_LOG_ERROR("Failed to generate random number");
            return "";
        }

        persistent_data::UserSubscription newSub(subValue->userSub);

        persistent_data::EventServiceStore::getInstance()
            .subscriptionsConfigMap.emplace(newSub.id, newSub);

        updateNoOfSubscribersCount();

        if constexpr (!BMCWEB_REDFISH_DBUS_LOG)
        {
            if (redfishLogFilePosition != 0)
            {
                cacheRedfishLogFile();
            }
        }
        // Update retry configuration.
        subValue->updateRetryConfig(retryAttempts, retryTimeoutInterval);

        // Set Subscription ID for back trace
        subValue->setSubscriptionId(id);

        return id;
    }

    std::string
        addSSESubscription(const std::shared_ptr<Subscription>& subValue,
                           std::string_view lastEventId)
    {
        std::string id = addSubscriptionInternal(subValue);

        if (!lastEventId.empty())
        {
            BMCWEB_LOG_INFO("Attempting to find message for last id {}",
                            lastEventId);
            boost::circular_buffer<Event>::iterator lastEvent =
                std::find_if(messages.begin(), messages.end(),
                             [&lastEventId](const Event& event) {
                                 return event.id == lastEventId;
                             });
            // Can't find a matching ID
            if (lastEvent == messages.end())
            {
                nlohmann::json msg = messages::eventBufferExceeded();
                // If the buffer overloaded, send all messages.
                subValue->sendEventToSubscriber(msg);
                lastEvent = messages.begin();
            }
            else
            {
                // Skip the last event the user already has
                lastEvent++;
            }

            for (boost::circular_buffer<Event>::const_iterator event =
                     lastEvent;
                 lastEvent != messages.end(); lastEvent++)
            {
                subValue->sendEventToSubscriber(event->message);
            }
        }
        return id;
    }

    std::string
        addPushSubscription(const std::shared_ptr<Subscription>& subValue)
    {
        std::string id = addSubscriptionInternal(subValue);

        updateSubscriptionData();
        return id;
    }

    bool isSubscriptionExist(const std::string& id)
    {
        auto obj = subscriptionsMap.find(id);
        return obj != subscriptionsMap.end();
    }

    bool deleteSubscription(const std::string& id)
    {
        auto obj = subscriptionsMap.find(id);
        if (obj == subscriptionsMap.end())
        {
            BMCWEB_LOG_WARNING("Could not find subscription with id {}", id);
            return false;
        }
        subscriptionsMap.erase(obj);
        auto& event = persistent_data::EventServiceStore::getInstance();
        auto persistentObj = event.subscriptionsConfigMap.find(id);
        if (persistentObj == event.subscriptionsConfigMap.end())
        {
            BMCWEB_LOG_ERROR("Subscription wasn't in persistent data");
            return true;
        }
        persistent_data::EventServiceStore::getInstance()
            .subscriptionsConfigMap.erase(persistentObj);
        updateNoOfSubscribersCount();
        updateSubscriptionData();

        return true;
    }

    void deleteSseSubscription(const crow::sse_socket::Connection& thisConn)
    {
        for (auto it = subscriptionsMap.begin(); it != subscriptionsMap.end();)
        {
            std::shared_ptr<Subscription> entry = it->second;
            bool entryIsThisConn = entry->matchSseId(thisConn);
            if (entryIsThisConn)
            {
                persistent_data::EventServiceStore::getInstance()
                    .subscriptionsConfigMap.erase(
                        it->second->getSubscriptionId());
                it = subscriptionsMap.erase(it);
                return;
            }
            it++;
        }
    }

    size_t getNumberOfSubscriptions() const
    {
        return subscriptionsMap.size();
    }

    size_t getNumberOfSSESubscriptions() const
    {
        auto size = std::ranges::count_if(
            subscriptionsMap,
            [](const std::pair<std::string, std::shared_ptr<Subscription>>&
                   entry) {
                return (entry.second->userSub.subscriptionType ==
                        subscriptionTypeSSE);
            });
        return static_cast<size_t>(size);
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

    bool sendTestEventLog()
    {
        for (const auto& it : subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            if (!entry->sendTestEventLog())
            {
                return false;
            }
        }
        return true;
    }

    void sendEvent(nlohmann::json::object_t eventMessage,
                   std::string_view origin, std::string_view resourceType)
    {
        eventMessage["EventId"] = eventId;

        eventMessage["EventTimestamp"] =
            redfish::time_utils::getDateTimeOffsetNow().first;
        eventMessage["OriginOfCondition"] = origin;

        // MemberId is 0 : since we are sending one event record.
        eventMessage["MemberId"] = "0";

        messages.push_back(Event(std::to_string(eventId), eventMessage));

        for (auto& it : subscriptionsMap)
        {
            std::shared_ptr<Subscription>& entry = it.second;
            if (!eventMatchesFilter(entry->userSub, eventMessage, resourceType))
            {
                BMCWEB_LOG_DEBUG("Filter didn't match");
                continue;
            }

            nlohmann::json::array_t eventRecord;
            eventRecord.emplace_back(eventMessage);

            nlohmann::json msgJson;

            msgJson["@odata.type"] = "#Event.v1_4_0.Event";
            msgJson["Name"] = "Event Log";
            msgJson["Id"] = eventId;
            msgJson["Events"] = std::move(eventRecord);

            std::string strMsg = msgJson.dump(
                2, ' ', true, nlohmann::json::error_handler_t::replace);
            entry->sendEventToSubscriber(std::move(strMsg));
            eventId++; // increment the eventId
        }
    }

    void resetRedfishFilePosition()
    {
        // Control would be here when Redfish file is created.
        // Reset File Position as new file is created
        redfishLogFilePosition = 0;
    }

    void cacheRedfishLogFile()
    {
        // Open the redfish file and read till the last record.

        std::ifstream logStream(redfishEventLogFile);
        if (!logStream.good())
        {
            BMCWEB_LOG_ERROR(" Redfish log file open failed ");
            return;
        }
        std::string logEntry;
        while (std::getline(logStream, logEntry))
        {
            redfishLogFilePosition = logStream.tellg();
        }
    }

    void readEventLogsFromFile()
    {
        std::ifstream logStream(redfishEventLogFile);
        if (!logStream.good())
        {
            BMCWEB_LOG_ERROR(" Redfish log file open failed");
            return;
        }

        std::vector<EventLogObjectsType> eventRecords;

        std::string logEntry;

        BMCWEB_LOG_DEBUG("Redfish log file: seek to {}",
                         static_cast<int>(redfishLogFilePosition));

        // Get the read pointer to the next log to be read.
        logStream.seekg(redfishLogFilePosition);

        while (std::getline(logStream, logEntry))
        {
            BMCWEB_LOG_DEBUG("Redfish log file: found new event log entry");
            // Update Pointer position
            redfishLogFilePosition = logStream.tellg();

            std::string idStr;
            if (!event_log::getUniqueEntryID(logEntry, idStr))
            {
                BMCWEB_LOG_DEBUG(
                    "Redfish log file: could not get unique entry id for {}",
                    logEntry);
                continue;
            }

            if (!serviceEnabled || noOfEventLogSubscribers == 0)
            {
                // If Service is not enabled, no need to compute
                // the remaining items below.
                // But, Loop must continue to keep track of Timestamp
                BMCWEB_LOG_DEBUG(
                    "Redfish log file: no subscribers / event service not enabled");
                continue;
            }

            std::string timestamp;
            std::string messageID;
            std::vector<std::string> messageArgs;
            if (event_log::getEventLogParams(logEntry, timestamp, messageID,
                                             messageArgs) != 0)
            {
                BMCWEB_LOG_DEBUG("Read eventLog entry params failed for {}",
                                 logEntry);
                continue;
            }

            eventRecords.emplace_back(idStr, timestamp, messageID, messageArgs);
        }

        if (!serviceEnabled || noOfEventLogSubscribers == 0)
        {
            BMCWEB_LOG_DEBUG("EventService disabled or no Subscriptions.");
            return;
        }

        if (eventRecords.empty())
        {
            // No Records to send
            BMCWEB_LOG_DEBUG("No log entries available to be transferred.");
            return;
        }

        for (const auto& it : subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            if (entry->userSub.eventFormatType == "Event")
            {
                entry->filterAndSendEventLogs(eventRecords);
            }
        }
    }

    static void watchRedfishEventLogFile()
    {
        if (!inotifyConn)
        {
            BMCWEB_LOG_ERROR("inotify Connection is not present");
            return;
        }

        static std::array<char, 1024> readBuffer;

        inotifyConn->async_read_some(
            boost::asio::buffer(readBuffer),
            [&](const boost::system::error_code& ec,
                const std::size_t& bytesTransferred) {
                if (ec == boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_DEBUG("Inotify was canceled (shutdown?)");
                    return;
                }
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Callback Error: {}", ec.message());
                    return;
                }

                BMCWEB_LOG_DEBUG("reading {} via inotify", bytesTransferred);

                std::size_t index = 0;
                while ((index + iEventSize) <= bytesTransferred)
                {
                    struct inotify_event event
                    {};
                    std::memcpy(&event, &readBuffer[index], iEventSize);
                    if (event.wd == dirWatchDesc)
                    {
                        if ((event.len == 0) ||
                            (index + iEventSize + event.len > bytesTransferred))
                        {
                            index += (iEventSize + event.len);
                            continue;
                        }

                        std::string fileName(&readBuffer[index + iEventSize]);
                        if (fileName != "redfish")
                        {
                            index += (iEventSize + event.len);
                            continue;
                        }

                        BMCWEB_LOG_DEBUG(
                            "Redfish log file created/deleted. event.name: {}",
                            fileName);
                        if (event.mask == IN_CREATE)
                        {
                            if (fileWatchDesc != -1)
                            {
                                BMCWEB_LOG_DEBUG(
                                    "Remove and Add inotify watcher on "
                                    "redfish event log file");
                                // Remove existing inotify watcher and add
                                // with new redfish event log file.
                                inotify_rm_watch(inotifyFd, fileWatchDesc);
                                fileWatchDesc = -1;
                            }

                            fileWatchDesc = inotify_add_watch(
                                inotifyFd, redfishEventLogFile, IN_MODIFY);
                            if (fileWatchDesc == -1)
                            {
                                BMCWEB_LOG_ERROR("inotify_add_watch failed for "
                                                 "redfish log file.");
                                return;
                            }

                            EventServiceManager::getInstance()
                                .resetRedfishFilePosition();
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
        BMCWEB_LOG_DEBUG("starting Event Log Monitor");

        inotifyConn.emplace(ioc);
        inotifyFd = inotify_init1(IN_NONBLOCK);
        if (inotifyFd == -1)
        {
            BMCWEB_LOG_ERROR("inotify_init1 failed.");
            return -1;
        }

        // Add watch on directory to handle redfish event log file
        // create/delete.
        dirWatchDesc = inotify_add_watch(inotifyFd, redfishEventLogDir,
                                         IN_CREATE | IN_MOVED_TO | IN_DELETE);
        if (dirWatchDesc == -1)
        {
            BMCWEB_LOG_ERROR(
                "inotify_add_watch failed for event log directory.");
            return -1;
        }

        // Watch redfish event log file for modifications.
        fileWatchDesc =
            inotify_add_watch(inotifyFd, redfishEventLogFile, IN_MODIFY);
        if (fileWatchDesc == -1)
        {
            BMCWEB_LOG_ERROR("inotify_add_watch failed for redfish log file.");
            // Don't return error if file not exist.
            // Watch on directory will handle create/delete of file.
        }

        // monitor redfish event log file
        inotifyConn->assign(inotifyFd);
        watchRedfishEventLogFile();

        return 0;
    }

    static void stopEventLogMonitor()
    {
        inotifyConn.reset();
    }

    static void getReadingsForReport(sdbusplus::message_t& msg)
    {
        if (msg.is_method_error())
        {
            BMCWEB_LOG_ERROR("TelemetryMonitor Signal error");
            return;
        }

        sdbusplus::message::object_path path(msg.get_path());
        std::string id = path.filename();
        if (id.empty())
        {
            BMCWEB_LOG_ERROR("Failed to get Id from path");
            return;
        }

        std::string interface;
        dbus::utility::DBusPropertiesMap props;
        std::vector<std::string> invalidProps;
        msg.read(interface, props, invalidProps);

        auto found = std::ranges::find_if(props, [](const auto& x) {
            return x.first == "Readings";
        });
        if (found == props.end())
        {
            BMCWEB_LOG_INFO("Failed to get Readings from Report properties");
            return;
        }

        const telemetry::TimestampReadings* readings =
            std::get_if<telemetry::TimestampReadings>(&found->second);
        if (readings == nullptr)
        {
            BMCWEB_LOG_INFO("Failed to get Readings from Report properties");
            return;
        }

        for (const auto& it :
             EventServiceManager::getInstance().subscriptionsMap)
        {
            Subscription& entry = *it.second;
            if (entry.userSub.eventFormatType == metricReportFormatType)
            {
                entry.filterAndSendReports(id, *readings);
            }
        }
    }

    void unregisterMetricReportSignal()
    {
        if (matchTelemetryMonitor)
        {
            BMCWEB_LOG_DEBUG("Metrics report signal - Unregister");
            matchTelemetryMonitor.reset();
            matchTelemetryMonitor = nullptr;
        }
    }

    void registerMetricReportSignal()
    {
        if (!serviceEnabled || matchTelemetryMonitor)
        {
            BMCWEB_LOG_DEBUG("Not registering metric report signal.");
            return;
        }

        BMCWEB_LOG_DEBUG("Metrics report signal - Register");
        std::string matchStr = "type='signal',member='PropertiesChanged',"
                               "interface='org.freedesktop.DBus.Properties',"
                               "arg0=xyz.openbmc_project.Telemetry.Report";

        matchTelemetryMonitor = std::make_shared<sdbusplus::bus::match_t>(
            *crow::connections::systemBus, matchStr, getReadingsForReport);
    }
};

} // namespace redfish
