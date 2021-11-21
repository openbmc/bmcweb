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
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/task_event_message_registry.hpp"

#include <sys/inotify.h>

#include <boost/asio/io_context.hpp>
#include <boost/container/flat_map.hpp>
#include <error_messages.hpp>
#include <event_service_store.hpp>
#include <http_client.hpp>
#include <persistent_data.hpp>
#include <random.hpp>
#include <server_sent_events.hpp>
#include <utils/json_utils.hpp>

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <memory>
#include <variant>

#include <boost/algorithm/string.hpp>

namespace redfish
{

using ReadingsObjType =
    std::vector<std::tuple<std::string, std::string, double, int32_t>>;

static constexpr const char* eventFormatType = "Event";
static constexpr const char* metricReportFormatType = "MetricReport";

static constexpr const char* eventServiceFile =
    "/var/lib/bmcweb/eventservice_config.json";

namespace message_registries
{
inline boost::beast::span<const MessageEntry>
    getRegistryFromPrefix(const std::string& registryName)
{
    if (task_event::header.registryPrefix == registryName)
    {
        return boost::beast::span<const MessageEntry>(task_event::registry);
    }
    if (openbmc::header.registryPrefix == registryName)
    {
        return boost::beast::span<const MessageEntry>(openbmc::registry);
    }
    if (base::header.registryPrefix == registryName)
    {
        return boost::beast::span<const MessageEntry>(base::registry);
    }
    return boost::beast::span<const MessageEntry>(openbmc::registry);
}
} // namespace message_registries

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
const Message*
    getMsgFromRegistry(const std::string& messageKey,
                       const boost::beast::span<const MessageEntry>& registry);

const Message* formatMessage(const std::string_view& messageID);

} // namespace message_registries

namespace event_log
{
bool getUniqueEntryID(const std::string& logEntry, std::string& entryID,
                             const bool firstEntry = true);

int getEventLogParams(const std::string& logEntry,
                             std::string& timestamp, std::string& messageID,
                             std::vector<std::string>& messageArgs);

void getRegistryAndMessageKey(const std::string& messageID,
                                     std::string& registryName,
                                     std::string& messageKey);

int formatEventLogEntry(const std::string& logEntryID,
                               const std::string& messageID,
                               const std::vector<std::string>& messageArgs,
                               std::string timestamp,
                               const std::string& customText,
                               nlohmann::json& logEntryJson);

} // namespace event_log
#endif

bool
    readSSEQueryParams(std::string sseFilter, std::string& formatType,
                       std::vector<std::string>& messageIds,
                       std::vector<std::string>& registryPrefixes,
                       std::vector<std::string>& metricReportDefinitions);

class Subscription : public persistent_data::UserSubscription
{
  public:
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    Subscription(const std::string& inHost, const std::string& inPort,
                 const std::string& inPath, const std::string& inUriProto);
    Subscription(const std::shared_ptr<boost::beast::tcp_stream>& adaptor);

    ~Subscription() = default;

    void sendEvent(const std::string& msg);

    void sendTestEventLog();

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    void filterAndSendEventLogs(
        const std::vector<EventLogObjectsType>& eventRecords);
#endif
    void filterAndSendReports(const std::string& id2,
                              const std::string& readingsTs,
                              const ReadingsObjType& readings);
    void updateRetryConfig(const uint32_t retryAttempts,
                           const uint32_t retryTimeoutInterval);
    void updateRetryPolicy();
    uint64_t getEventSeqNum();

  private:
    uint64_t eventSeqNum;
    std::string host;
    std::string port;
    std::string path;
    std::string uriProto;
    std::shared_ptr<crow::HttpClient> conn = nullptr;
    std::shared_ptr<crow::ServerSentEvents> sseConn = nullptr;
};

class EventServiceManager
{
  private:
    bool serviceEnabled;
    uint32_t retryAttempts;
    uint32_t retryTimeoutInterval;

    EventServiceManager();

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

    static EventServiceManager& getInstance();

    void initConfig();
    void loadOldBehavior();
    void updateSubscriptionData();
    void setEventServiceConfig(const persistent_data::EventServiceConfig& cfg);
    void updateNoOfSubscribersCount();
    std::shared_ptr<Subscription> getSubscription(const std::string& id);
    std::string addSubscription(const std::shared_ptr<Subscription>& subValue,
                                const bool updateFile = true);
    bool isSubscriptionExist(const std::string& id);
    void deleteSubscription(const std::string& id);
    size_t getNumberOfSubscriptions();
    std::vector<std::string> getAllIDs();
    bool isDestinationExist(const std::string& destUrl);
    void sendTestEventLog();
    void sendEvent(const nlohmann::json& eventMessageIn,
                   const std::string& origin, const std::string& resType);
    void sendBroadcastMsg(const std::string& broadcastMsg);

#ifndef BMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES
    void cacheLastEventTimestamp();
    void readEventLogsFromFile();
    static void watchRedfishEventLogFile();
    static int startEventLogMonitor(boost::asio::io_context& ioc);
#endif

    void getMetricReading(const std::string& service,
                          const std::string& objPath, const std::string& intf);
    void unregisterMetricReportSignal();
    void registerMetricReportSignal();

    bool validateAndSplitUrl(const std::string& destUrl, std::string& urlProto,
                             std::string& host, std::string& port,
                             std::string& path);
};

} // namespace redfish
