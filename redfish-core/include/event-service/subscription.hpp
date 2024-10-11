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

#include "event-service/event-log-objects-type.hpp"
#include "event-service/event-log.hpp"
#include "event_service_store.hpp"
#include "filter_expr_executor.hpp"
#include "generated/enums/log_entry.hpp"
#include "http_client.hpp"
#include "metric_report.hpp"
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
#include <memory>
#include <span>
#include <string>

namespace redfish
{

class Subscription : public persistent_data::UserSubscription
{
  public:
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

    Subscription(const boost::urls::url_view_base& url,
                 boost::asio::io_context& ioc) :
        policy(std::make_shared<crow::ConnectionPolicy>())
    {
        destinationUrl = url;
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
            client->sendData(
                std::move(msg), destinationUrl,
                static_cast<ensuressl::VerifyCertificate>(verifyCertificate),
                httpHeaders, boost::beast::http::verb::post);
            return true;
        }

        if (sseConn != nullptr)
        {
            eventSeqNum++;
            sseConn->sendSseEvent(std::to_string(eventSeqNum), msg);
        }
        return true;
    }

    bool eventMatchesFilter(const nlohmann::json::object_t& eventMessage,
                            std::string_view resType)
    {
        // If resourceTypes list is empty, assume all
        if (!resourceTypes.empty())
        {
            // Search the resourceTypes list for the subscription.
            auto resourceTypeIndex = std::ranges::find_if(
                resourceTypes, [resType](const std::string& rtEntry) {
                    return rtEntry == resType;
                });
            if (resourceTypeIndex == resourceTypes.end())
            {
                BMCWEB_LOG_DEBUG("Not subscribed to this resource");
                return false;
            }
            BMCWEB_LOG_DEBUG("ResourceType {} found in the subscribed list",
                             resType);
        }

        // If registryPrefixes list is empty, don't filter events
        // send everything.
        if (!registryPrefixes.empty())
        {
            auto eventJson = eventMessage.find("MessageId");
            if (eventJson == eventMessage.end())
            {
                return false;
            }

            const std::string* messageId =
                eventJson->second.get_ptr<const std::string*>();
            if (messageId == nullptr)
            {
                BMCWEB_LOG_ERROR("MessageId wasn't a string???");
                return false;
            }

            std::string registry;
            std::string messageKey;
            event_log::getRegistryAndMessageKey(*messageId, registry,
                                                messageKey);

            auto obj = std::ranges::find(registryPrefixes, registry);
            if (obj == registryPrefixes.end())
            {
                return false;
            }
        }

        if (!originResources.empty())
        {
            auto eventJson = eventMessage.find("OriginOfCondition");
            if (eventJson == eventMessage.end())
            {
                return false;
            }

            const std::string* originOfCondition =
                eventJson->second.get_ptr<const std::string*>();
            if (originOfCondition == nullptr)
            {
                BMCWEB_LOG_ERROR("OriginOfCondition wasn't a string???");
                return false;
            }

            auto obj = std::ranges::find(originResources, *originOfCondition);

            if (obj == originResources.end())
            {
                return false;
            }
        }

        // If registryMsgIds list is empty, assume all
        if (!registryMsgIds.empty())
        {
            auto eventJson = eventMessage.find("MessageId");
            if (eventJson == eventMessage.end())
            {
                BMCWEB_LOG_DEBUG("'MessageId' not present");
                return false;
            }

            const std::string* messageId =
                eventJson->second.get_ptr<const std::string*>();
            if (messageId == nullptr)
            {
                BMCWEB_LOG_ERROR("EventType wasn't a string???");
                return false;
            }

            std::string registry;
            std::string messageKey;
            event_log::getRegistryAndMessageKey(*messageId, registry,
                                                messageKey);

            BMCWEB_LOG_DEBUG("extracted registry {}", registry);
            BMCWEB_LOG_DEBUG("extracted message key {}", messageKey);

            auto obj = std::ranges::find(
                registryMsgIds, std::format("{}.{}", registry, messageKey));
            if (obj == registryMsgIds.end())
            {
                BMCWEB_LOG_DEBUG("did not find registry {} in registryMsgIds",
                                 registry);
                BMCWEB_LOG_DEBUG("registryMsgIds has {} entries",
                                 registryMsgIds.size());
                return false;
            }
        }

        if (filter)
        {
            if (!memberMatches(eventMessage, *filter))
            {
                BMCWEB_LOG_DEBUG("Filter didn't match");
                return false;
            }
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
        logEntryJson["MemberId"] = 0;
        logEntryJson["MessageArgs"] = nlohmann::json::array();
        logEntryJson["EventTimestamp"] =
            redfish::time_utils::getDateTimeOffsetNow().first;
        logEntryJson["Context"] = customText;

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
                    logEntry.timestamp, customText, bmcLogEntry) != 0)
            {
                BMCWEB_LOG_DEBUG("Read eventLog entry failed");
                continue;
            }

            if (!eventMatchesFilter(bmcLogEntry, ""))
            {
                BMCWEB_LOG_DEBUG("Event {} did not match the filter",
                                 nlohmann::json(bmcLogEntry).dump());
                continue;
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
        if (!metricReportDefinitions.empty())
        {
            if (std::ranges::find(metricReportDefinitions, mrdUri.buffer()) ==
                metricReportDefinitions.end())
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
        if (!customText.empty())
        {
            msg["Context"] = customText;
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

} // namespace redfish
