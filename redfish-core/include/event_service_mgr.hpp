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
#include <systemd/sd-event.h>

#include <boost/container/flat_map.hpp>
#include <cstdlib>
#include <ctime>
#include <error_messages.hpp>
#include <http_client.hpp>
#include <memory>
#include <sdbusplus/asio/sd_event.hpp>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{
#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
constexpr const char* redfishEventLogFile = "/var/log/redfish";
constexpr const uint32_t inotifyFileAction = IN_MODIFY;

struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};

using EventPtr = std::unique_ptr<sd_event, EventDeleter>;
static std::unique_ptr<sd_event, EventDeleter> eventPtr;

namespace message_registries
{
static const Message*
    getMsgFromRegistry(const std::string& messageKey,
                       const boost::beast::span<const MessageEntry> registry)
{
    boost::beast::span<const MessageEntry>::const_iterator messageIt =
        std::find_if(registry.cbegin(), registry.cend(),
                     [&messageKey](const MessageEntry& messageEntry) {
                         return !std::strcmp(messageEntry.first,
                                             messageKey.c_str());
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
bool getUniqueEntryID(const std::string& logEntry, std::string& entryID,
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
    }
    // If the timestamp isn't unique, increment the index
    if (curTs == prevTs)
    {
        index++;
    }
    else
    {
        // Otherwise, reset it
        index = 0;
    }
    // Save the timestamp
    prevTs = curTs;

    entryID = std::to_string(curTs);
    if (index > 0)
    {
        entryID += "_" + std::to_string(index);
    }
    return true;
}

int getEventLogParams(const std::string logEntry, std::string& timestamp,
                      std::string& messageID,
                      boost::beast::span<std::string>& messageArgs)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(" ");
    if (space == std::string::npos)
    {
        return 1;
    }
    timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(" ", space);
    if (entryStart == std::string::npos)
    {
        return 1;
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
        return 1;
    }
    messageID = logEntryFields[0];

    // Get the MessageArgs from the log if there are any
    if (logEntryFields.size() > 1)
    {
        std::string& messageArgsStart = logEntryFields[1];
        // If the first string is empty, assume there are no MessageArgs
        std::size_t messageArgsSize = 0;
        if (!messageArgsStart.empty())
        {
            messageArgsSize = logEntryFields.size() - 1;
        }

        messageArgs = boost::beast::span(&messageArgsStart, messageArgsSize);
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
    registryName = fields[0];
    messageKey = fields[3];
}

int formatEventLogEntry(const std::string& logEntryID,
                        const std::string& messageID,
                        const boost::beast::span<std::string>& messageArgs,
                        std::string& timestamp, const std::string customText,
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
    std::size_t dot = timestamp.find_first_of(".");
    std::size_t plus = timestamp.find_first_of("+");
    if (dot != std::string::npos && plus != std::string::npos)
    {
        timestamp.erase(dot, plus - dot);
    }

    // Fill in the log entry with the gathered data
    logEntryJson = {{"EventId", logEntryID},
                    {"EventType", "Event"},
                    {"Severity", std::move(severity)},
                    {"Message", std::move(msg)},
                    {"MessageId", std::move(messageID)},
                    {"MessageArgs", std::move(messageArgs)},
                    {"EventTimestamp", std::move(timestamp)},
                    {"Context", customText}};
    return 0;
}
#endif

} // namespace event_log

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
    std::vector<nlohmann::json> httpHeaders; // key-value pair

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    Subscription(const std::string& _host, const std::string& _port,
                 const std::string& _path, const std::string& _uriProto) :
        host(_host),
        port(_port), path(_path), uriProto(_uriProto)
    {
        conn = std::make_shared<crow::HttpClient>(
            crow::connections::systemBus->get_io_context(), host, port);
        eventSeqNum = 1;
    }
    ~Subscription()
    {
    }

    void sendEvent(const std::string& msg)
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
        conn->doConnectAndSend(path, msg);
    }

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    void filterAndSendEventLogs(const std::vector<std::string>& eventRecords)
    {
        nlohmann::json logEntryArray;

        bool firstEntry = true;
        for (const std::string& logEntry : eventRecords)
        {
            std::string idStr;
            if (!event_log::getUniqueEntryID(logEntry, idStr, firstEntry))
            {
                continue;
            }
            if (firstEntry)
            {
                firstEntry = false;
            }

            std::string timestamp;
            std::string messageID;
            boost::beast::span<std::string> messageArgs;
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
            BMCWEB_LOG_DEBUG << "No log entries for sending";
            return;
        }

        nlohmann::json msg = {{"@odata.type", "#Event.v1_4_0.Event"},
                              {"Id", std::to_string(eventSeqNum)},
                              {"Name", "Event Log"},
                              {"Events", logEntryArray}};

        this->sendEvent(msg.dump());
        this->eventSeqNum++;

        return;
    }
#endif

  private:
    uint64_t eventSeqNum;
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<crow::HttpClient> conn;
};

class EventSrvManager
{
  private:
    EventSrvManager(const EventSrvManager&) = delete;
    EventSrvManager& operator=(const EventSrvManager&) = delete;
    EventSrvManager(EventSrvManager&&) = delete;
    EventSrvManager& operator=(EventSrvManager&&) = delete;

    EventSrvManager()
    {
        // TODO: Read the persistent data from store and populate
        // Poulating with default.
        enabled = true;
        retryAttempts = 3;
        retryTimeoutInterval = 30; // seconds
    }

    std::string lastEventTStr;
    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        subscriptionsMap;

  public:
    bool enabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;

    static EventSrvManager& getInstance()
    {
        static EventSrvManager handler;
        return handler;
    }

    void updateSubscriptionData()
    {
        // Persist the config and subscription data.
        // TODO: subscriptionsMap & configData need to be
        // written to Persist store.
        return;
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

    std::string addSubscription(const std::shared_ptr<Subscription> subValue)
    {
        std::srand(static_cast<uint32_t>(std::time(0)));
        std::string id;

        int retry = 4;
        while (--retry)
        {
            id = std::to_string(std::rand());
            auto inserted = subscriptionsMap.insert(std::pair(id, subValue));
            if (inserted.second)
            {
                break;
            }
        };

        if (retry <= 0)
        {
            BMCWEB_LOG_ERROR << "Filed to generate random number";
            return std::string("");
        }

        updateSubscriptionData();

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
        if (lastEventTStr.empty())
        {
            setLastEventTimestamp();
        }
#endif
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

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    void setLastEventTimestamp()
    {
        std::ifstream logStream(redfishEventLogFile);
        if (!logStream.good())
        {
            BMCWEB_LOG_ERROR << " Redfish log file open failed \n";
            return;
        }
        std::string logEntry;
        while (std::getline(logStream, logEntry))
        {
            size_t space = logEntry.find_first_of(" ");
            if (space == std::string::npos)
            {
                // Shouldn't enter here but lets skip it.
                BMCWEB_LOG_DEBUG << "Error format log present";
                continue;
            }
            lastEventTStr = logEntry.substr(0, space);
        }
        BMCWEB_LOG_DEBUG << "Last Event time stamp set: " << lastEventTStr;
    }

    void readEventLogsFromFile()
    {
        if (!getNumberOfSubscriptions())
        {
            // no subscriptions. Just return.
            BMCWEB_LOG_DEBUG << "No Subscriptions\n";
            return;
        }
        std::ifstream logStream(redfishEventLogFile);
        if (!logStream.good())
        {
            BMCWEB_LOG_ERROR << " Redfish log file open failed";
            return;
        }

        std::vector<std::string> eventRecords;

        bool startLogCollection = false;
        std::string logEntry;
        while (std::getline(logStream, logEntry))
        {
            if (!startLogCollection)
            {
                if (boost::starts_with(logEntry, lastEventTStr))
                {
                    startLogCollection = true;
                }
                continue;
            }
            size_t space = logEntry.find_first_of(" ");
            if (space == std::string::npos)
            {
                // Shouldn't enter here but lets skip it.
                BMCWEB_LOG_DEBUG << "Error format log present";
                continue;
            }
            lastEventTStr = logEntry.substr(0, space);
            eventRecords.push_back(logEntry);
        }

        for (const auto& it : this->subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            entry->filterAndSendEventLogs(eventRecords);
        }
    }

    static int notificationHandler(sd_event_source* eventSource,
                                   const struct inotify_event* event,
                                   void* data)
    {
        BMCWEB_LOG_DEBUG << "inotification received on redfish event log";
        if (!(event->mask & inotifyFileAction))
        {
            return 0;
        }

        if (data)
        {
            auto handler = static_cast<EventSrvManager*>(data);
            handler->readEventLogsFromFile();
        }
        return 0;
    }

    static int startEventLogMonitor()
    {
        int rc = 0;
        sd_event* event;
        rc = sd_event_default(&event);
        if (rc < 0)
        {
            BMCWEB_LOG_ERROR << "Unable to acquire event loop object";
            return rc;
        }

        eventPtr = EventPtr(event);

        // Add inotify on redfish event log file
        rc = sd_event_add_inotify(eventPtr.get(), nullptr, redfishEventLogFile,
                                  inotifyFileAction, notificationHandler,
                                  &EventSrvManager::getInstance());
        if (rc < 0)
        {
            BMCWEB_LOG_ERROR << "inotify on redfish event log failed";
            return rc;
        }

        return rc;
    }
#endif
};

} // namespace redfish
