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
#include "dbus_log_watcher.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "event_log.hpp"
#include "event_matches_filter.hpp"
#include "event_service_store.hpp"
#include "filesystem_log_watcher.hpp"
#include "metric_report.hpp"
#include "ossl_random.hpp"
#include "persistent_data.hpp"
#include "subscription.hpp"
#include "utility.hpp"
#include "utils/dbus_event_log_entry.hpp"
#include "utils/json_utils.hpp"
#include "utils/time_utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url_view_base.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace redfish
{

static constexpr const char* eventFormatType = "Event";
static constexpr const char* metricReportFormatType = "MetricReport";

static constexpr const char* eventServiceFile =
    "/var/lib/bmcweb/eventservice_config.json";

class EventServiceManager
{
  private:
    bool serviceEnabled = false;
    uint32_t retryAttempts = 0;
    uint32_t retryTimeoutInterval = 0;

    size_t noOfEventLogSubscribers{0};
    size_t noOfMetricReportSubscribers{0};
    std::optional<DbusEventLogMonitor> dbusEventLogMonitor;
    std::optional<DbusTelemetryMonitor> matchTelemetryMonitor;
    std::optional<FilesystemLogWatcher> filesystemLogMonitor;
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
            std::shared_ptr<persistent_data::UserSubscription> newSub =
                it.second;

            boost::system::result<boost::urls::url> url =
                boost::urls::parse_absolute_uri(newSub->destinationUrl);

            if (!url)
            {
                BMCWEB_LOG_ERROR(
                    "Failed to validate and split destination url");
                continue;
            }
            std::shared_ptr<Subscription> subValue =
                std::make_shared<Subscription>(newSub, *url, ioc);
            std::string id = subValue->userSub->id;
            subValue->deleter = [id]() {
                EventServiceManager::getInstance().deleteSubscription(id);
            };

            subscriptionsMap.emplace(id, subValue);

            updateNoOfSubscribersCount();

            // Update retry configuration.
            subValue->updateRetryConfig(retryAttempts, retryTimeoutInterval);

            // schedule a heartbeat if sendHeartbeat was set to true
            if (subValue->userSub->sendHeartbeat)
            {
                subValue->scheduleNextHeartbeatEvent();
            }
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
        if (obj == nullptr)
        {
            return;
        }
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
                                .subscriptionsConfigMap.insert(std::pair(
                                    id, std::make_shared<
                                            persistent_data::UserSubscription>(
                                            newSub)));
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

        if (serviceEnabled)
        {
            if (noOfEventLogSubscribers > 0U)
            {
                if constexpr (BMCWEB_REDFISH_DBUS_LOG)
                {
                    if (!dbusEventLogMonitor)
                    {
                        if constexpr (
                            BMCWEB_EXPERIMENTAL_REDFISH_DBUS_LOG_SUBSCRIPTION)
                        {
                            dbusEventLogMonitor.emplace();
                        }
                    }
                }
                else
                {
                    if (!filesystemLogMonitor)
                    {
                        filesystemLogMonitor.emplace(ioc);
                    }
                }
            }
            else
            {
                dbusEventLogMonitor.reset();
                filesystemLogMonitor.reset();
            }

            if (noOfMetricReportSubscribers > 0U)
            {
                if (!matchTelemetryMonitor)
                {
                    matchTelemetryMonitor.emplace();
                }
            }
            else
            {
                matchTelemetryMonitor.reset();
            }
        }
        else
        {
            matchTelemetryMonitor.reset();
            dbusEventLogMonitor.reset();
            filesystemLogMonitor.reset();
        }

        if (serviceEnabled != cfg.enabled)
        {
            serviceEnabled = cfg.enabled;
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
            if (entry->userSub->eventFormatType == eventFormatType)
            {
                eventLogSubCount++;
            }
            else if (entry->userSub->eventFormatType == metricReportFormatType)
            {
                metricReportSubCount++;
            }
        }
        noOfEventLogSubscribers = eventLogSubCount;
        if (eventLogSubCount > 0U)
        {
            if constexpr (BMCWEB_REDFISH_DBUS_LOG)
            {
                if (!dbusEventLogMonitor &&
                    BMCWEB_EXPERIMENTAL_REDFISH_DBUS_LOG_SUBSCRIPTION)
                {
                    dbusEventLogMonitor.emplace();
                }
            }
            else
            {
                if (!filesystemLogMonitor)
                {
                    filesystemLogMonitor.emplace(ioc);
                }
            }
        }
        else
        {
            dbusEventLogMonitor.reset();
            filesystemLogMonitor.reset();
        }

        noOfMetricReportSubscribers = metricReportSubCount;
        if (metricReportSubCount > 0U)
        {
            if (!matchTelemetryMonitor)
            {
                matchTelemetryMonitor.emplace();
            }
        }
        else
        {
            matchTelemetryMonitor.reset();
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

        // Set Subscription ID for back trace
        subValue->userSub->id = id;

        persistent_data::EventServiceStore::getInstance()
            .subscriptionsConfigMap.emplace(id, subValue->userSub);

        updateNoOfSubscribersCount();

        // Update retry configuration.
        subValue->updateRetryConfig(retryAttempts, retryTimeoutInterval);

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
        subValue->deleter = [id]() {
            EventServiceManager::getInstance().deleteSubscription(id);
        };
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
                    .subscriptionsConfigMap.erase(entry->userSub->id);
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
                return (entry.second->userSub->subscriptionType ==
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

    static void
        sendEventsToSubs(const std::vector<EventLogObjectsType>& eventRecords)
    {
        for (const auto& it :
             EventServiceManager::getInstance().subscriptionsMap)
        {
            Subscription& entry = *it.second;
            entry.filterAndSendEventLogs(eventRecords);
        }
    }

    static void sendTelemetryReportToSubs(
        const std::string& reportId, const telemetry::TimestampReadings& var)
    {
        for (const auto& it :
             EventServiceManager::getInstance().subscriptionsMap)
        {
            Subscription& entry = *it.second;
            entry.filterAndSendReports(reportId, var);
        }
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
            if (!eventMatchesFilter(*entry->userSub, eventMessage,
                                    resourceType))
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
        }
        eventId++; // increment the eventId
    }
};

} // namespace redfish
