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
#include "filter_expr_executor.hpp"
#include "logging.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <ctime>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

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
                logEntry.timestamp, userSub->customText, bmcLogEntry) != 0)
        {
            BMCWEB_LOG_DEBUG("Read eventLog entry failed");
            continue;
        }

        if (!eventMatchesFilter(*userSub, bmcLogEntry, ""))
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

} // namespace redfish
