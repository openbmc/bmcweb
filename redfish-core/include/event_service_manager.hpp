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

#include <boost/container/flat_map.hpp>
#include <cstdlib>
#include <ctime>
#include <error_messages.hpp>
#include <http_client.hpp>
#include <memory>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

using ReadingsObjType =
    std::vector<std::tuple<std::string, std::string, double, std::string>>;

static constexpr const char* eventFormatType = "Event";
static constexpr const char* metricReportFormatType = "MetricReport";

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
constexpr const char* redfishEventLogFile = "/var/log/redfish";
constexpr const uint32_t inotifyFileAction = IN_MODIFY;
std::shared_ptr<boost::asio::posix::stream_descriptor> inotifyConn = nullptr;

// <ID, timestamp, RedfishLogId, registryPrefix, MessageId, MessageArgs>
using EventLogObjectsType =
    std::tuple<std::string, std::string, std::string, std::string, std::string,
               boost::beast::span<std::string>>;

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

int getEventLogParams(const std::string& logEntry, std::string& timestamp,
                      std::string& messageID,
                      boost::beast::span<std::string>& messageArgs)
{
    // The redfish log format is "<Timestamp> <MessageId>,<MessageArgs>"
    // First get the Timestamp
    size_t space = logEntry.find_first_of(" ");
    if (space == std::string::npos)
    {
        return -EINVAL;
    }
    timestamp = logEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = logEntry.find_first_not_of(" ", space);
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
    if (fields.size() == 4)
    {
        registryName = fields[0];
        messageKey = fields[3];
    }
}

int formatEventLogEntry(const std::string& logEntryID,
                        const std::string& messageID,
                        const boost::beast::span<std::string>& messageArgs,
                        std::string timestamp, const std::string customText,
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
    std::vector<nlohmann::json> metricReportDefinitions;

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
            crow::connections::systemBus->get_io_context(), host, port, path);
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
        conn->setHeaders(reqHeaders);
        conn->sendData(msg);
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
        this->eventSeqNum++;
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
            const boost::beast::span<std::string>& messageArgs =
                std::get<5>(logEntry);

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
        this->eventSeqNum++;
    }
#endif

    void filterAndSendReports(const std::string& id,
                              const std::string& readingsTs,
                              const ReadingsObjType& readings)
    {
        std::string metricReportDef =
            "/redfish/v1/TelemetryService/MetricReportDefinitions/" + id;

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

            entry = {{"MetricId", std::get<0>(it)},
                     {"MetricProperty", std::get<1>(it)},
                     {"MetricValue", std::to_string(std::get<2>(it))},
                     {"Timestamp", std::get<3>(it)}};
        }

        nlohmann::json msg = {
            {"@odata.id", "/redfish/v1/TelemetryService/MetricReports/" + id},
            {"@odata.type", "#MetricReport.v1_3_0.MetricReport"},
            {"Id", id},
            {"Name", id},
            {"Timestamp", readingsTs},
            {"MetricReportDefinition", {{"@odata.id", metricReportDef}}},
            {"MetricValues", metricValuesArray}};

        this->sendEvent(msg.dump());
    }

  private:
    uint64_t eventSeqNum;
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<crow::HttpClient> conn;
};

class EventServiceManager
{
  private:
    EventServiceManager(const EventServiceManager&) = delete;
    EventServiceManager& operator=(const EventServiceManager&) = delete;
    EventServiceManager(EventServiceManager&&) = delete;
    EventServiceManager& operator=(EventServiceManager&&) = delete;

    EventServiceManager() : noOfMetricReportSubscribers(0)
    {
        // TODO: Read the persistent data from store and populate.
        // Populating with default.
        enabled = true;
        retryAttempts = 3;
        retryTimeoutInterval = 30; // seconds
    }

    std::string lastEventTStr;
    size_t noOfMetricReportSubscribers;
    std::shared_ptr<sdbusplus::bus::match::match> matchTelemetryMonitor;
    boost::container::flat_map<std::string, std::shared_ptr<Subscription>>
        subscriptionsMap;

  public:
    bool enabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;

    static EventServiceManager& getInstance()
    {
        static EventServiceManager handler;
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

        int retry = 3;
        while (retry)
        {
            id = std::to_string(std::rand());
            auto inserted = subscriptionsMap.insert(std::pair(id, subValue));
            if (inserted.second)
            {
                break;
            }
            --retry;
        };

        if (retry <= 0)
        {
            BMCWEB_LOG_ERROR << "Failed to generate random number";
            return std::string("");
        }

        if (subValue->eventFormatType == metricReportFormatType)
        {
            // If it is first entry,  Register Metrics report signal.
            if ((++noOfMetricReportSubscribers == 1))
            {
                registerMetricReportSignal();
            }
        }

        updateSubscriptionData();

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
        if (lastEventTStr.empty())
        {
            cacheLastEventTimestamp();
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
            std::shared_ptr<Subscription> entry = obj->second;
            if (entry->eventFormatType == metricReportFormatType)
            {
                if ((--noOfMetricReportSubscribers == 0))
                {
                    unregisterMetricReportSignal();
                }
            }

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

    void sendTestEventLog()
    {
        for (const auto& it : this->subscriptionsMap)
        {
            std::shared_ptr<Subscription> entry = it.second;
            entry->sendTestEventLog();
        }
    }

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    void cacheLastEventTimestamp()
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
                BMCWEB_LOG_DEBUG << "Invalid log entry found.";
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

        std::vector<EventLogObjectsType> eventRecords;

        bool startLogCollection = false;
        bool firstEntry = true;

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

            std::string idStr;
            if (!event_log::getUniqueEntryID(logEntry, idStr, firstEntry))
            {
                continue;
            }
            firstEntry = false;

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
        if (inotifyConn == nullptr)
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
                while ((index + sizeof(inotify_event)) <= bytesTransferred)
                {
                    struct inotify_event event;
                    std::memcpy(&event, &readBuffer[index],
                                sizeof(inotify_event));
                    if (event.mask == inotifyFileAction)
                    {
                        EventServiceManager::getInstance()
                            .readEventLogsFromFile();
                    }
                    index += (sizeof(inotify_event) + event.len);
                }

                watchRedfishEventLogFile();
            });
    }

    static int startEventLogMonitor(boost::asio::io_service& ioc)
    {
        inotifyConn =
            std::make_shared<boost::asio::posix::stream_descriptor>(ioc);
        int fd = inotify_init1(IN_NONBLOCK);
        if (fd == -1)
        {
            BMCWEB_LOG_ERROR << "inotify_init1 failed.";
            return -1;
        }
        auto wd = inotify_add_watch(fd, redfishEventLogFile, inotifyFileAction);
        if (wd == -1)
        {
            BMCWEB_LOG_ERROR
                << "inotify_add_watch failed for redfish log file.";
            return -1;
        }

        // monitor redfish event log file
        inotifyConn->assign(fd);
        watchRedfishEventLogFile();

        return 0;
    }

#endif

    void getMetricReading(const std::string& service,
                          const std::string& objPath, const std::string& intf)
    {
        std::size_t found = objPath.find_last_of("/");
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
                    std::string, std::variant<std::string, ReadingsObjType>>&
                    resp) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "D-Bus call failed to GetAll metric readings.";
                    return;
                }

                const std::string* timestampPtr =
                    std::get_if<std::string>(&resp["Timestamp"]);
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
                        entry->filterAndSendReports(idStr, *timestampPtr,
                                                    *readingsPtr);
                    }
                }
            },
            service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
            intf);
    }

    void unregisterMetricReportSignal()
    {
        BMCWEB_LOG_DEBUG << "Metrics report signal - Unregister";
        matchTelemetryMonitor.reset();
        matchTelemetryMonitor = nullptr;
    }

    void registerMetricReportSignal()
    {
        if (matchTelemetryMonitor)
        {
            BMCWEB_LOG_DEBUG << "Metrics report signal - Already registered.";
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
};

} // namespace redfish
