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

#include "subscription.hpp"

#include "event_logs_object_type.hpp"
#include "event_matches_filter.hpp"
#include "event_service_manager.hpp"
#include "event_service_store.hpp"
#include "filter_expr_executor.hpp"
#include "generated/enums/log_entry.hpp"
#include "http_client.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "metric_report.hpp"
#include "server_sent_event.hpp"
#include "ssl_key_handler.hpp"
#include "utils/time_utils.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/errc.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

Subscription::Subscription(const persistent_data::UserSubscription& userSubIn,
                           const boost::urls::url_view_base& url,
                           boost::asio::io_context& ioc) :
    userSub(userSubIn), policy(std::make_shared<crow::ConnectionPolicy>())
{
    userSub.destinationUrl = url;
    client.emplace(ioc, policy);
    // Subscription constructor
    policy->invalidResp = retryRespHandler;
}

Subscription::Subscription(crow::sse_socket::Connection& connIn) :
    sseConn(&connIn)
{}

// callback for subscription sendData
void Subscription::resHandler(const std::shared_ptr<Subscription>& /*unused*/,
                              const crow::Response& res)
{
    BMCWEB_LOG_DEBUG("Response handled with return code: {}", res.resultInt());

    if (!client)
    {
        BMCWEB_LOG_ERROR(
            "Http client wasn't filled but http client callback was called.");
        return;
    }

    if (userSub.retryPolicy != "TerminateAfterRetries")
    {
        return;
    }
    if (client->isTerminated())
    {
        if (deleter)
        {
            BMCWEB_LOG_INFO("Subscription {} is deleted after MaxRetryAttempts",
                            userSub.id);
            deleter();
        }
    }
}

bool Subscription::sendEventToSubscriber(std::string&& msg)
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
        client->sendDataWithCallback(
            std::move(msg), userSub.destinationUrl,
            static_cast<ensuressl::VerifyCertificate>(
                userSub.verifyCertificate),
            userSub.httpHeaders, boost::beast::http::verb::post,
            std::bind_front(&Subscription::resHandler, this,
                            shared_from_this()));
        return true;
    }

    if (sseConn != nullptr)
    {
        eventSeqNum++;
        sseConn->sendSseEvent(std::to_string(eventSeqNum), msg);
    }
    return true;
}

bool Subscription::sendTestEventLog()
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

void Subscription::filterAndSendEventLogs(
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

void Subscription::filterAndSendReports(const std::string& reportId,
                                        const telemetry::TimestampReadings& var)
{
    boost::urls::url mrdUri = boost::urls::format(
        "/redfish/v1/TelemetryService/MetricReportDefinitions/{}", reportId);

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

void Subscription::updateRetryConfig(uint32_t retryAttempts,
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

uint64_t Subscription::getEventSeqNum() const
{
    return eventSeqNum;
}

bool Subscription::matchSseId(const crow::sse_socket::Connection& thisConn)
{
    return &thisConn == sseConn;
}

// Check used to indicate what response codes are valid as part of our retry
// policy.  2XX is considered acceptable
boost::system::error_code Subscription::retryRespHandler(unsigned int respCode)
{
    BMCWEB_LOG_DEBUG("Checking response code validity for SubscriptionEvent");
    if ((respCode < 200) || (respCode >= 300))
    {
        return boost::system::errc::make_error_code(
            boost::system::errc::result_out_of_range);
    }

    // Return 0 if the response code is valid
    return boost::system::errc::make_error_code(boost::system::errc::success);
}

} // namespace redfish
