/*
// Copyright (c) 2018 Intel Corporation
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

#include <boost/algorithm/string.hpp>
#include <boost/beast/http.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/date_time.hpp>
#include <sdbusplus/bus/match.hpp>
#include <variant>

namespace redfish
{
constexpr const char* eventLogFile = "/var/log/redfish";
constexpr const uint32_t eventLogFileAction = IN_MODIFY;

struct EventDeleter
{
    void operator()(sd_event* event) const
    {
        event = sd_event_unref(event);
    }
};

using EventPtr = std::unique_ptr<sd_event, EventDeleter>;
static std::unique_ptr<sd_event, EventDeleter> eventPtr;

static std::vector<crow::Request::Adaptor> connections;

using response_type =
    boost::beast::http::response<boost::beast::http::string_body>;

class SSEvents : public Node
{
  public:
    SSEvents(CrowApp& app) : Node(app, "/redfish/v1/SSEvents/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        connections.emplace_back(std::move(req.socket()));
    }
};

class EventHandler
{
  private:
    std::optional<boost::beast::http::response_serializer<
        boost::beast::http::string_body>>
        serializer;

    std::optional<response_type> stringResponse;

    EventHandler(){};

    uint64_t eventId{0};

    // timestamp is read as string and converted to integer using
    // strtoll which returns signed int.
    int64_t lastTS{0};

  public:
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;

    static EventHandler& getInstance()
    {
        static EventHandler handler;
        return handler;
    }

    void sendEvents(const std::string& data)
    {
        stringResponse.emplace(response_type{});
        stringResponse->set("Content-Type", "text/event-stream");
        stringResponse->body() += data;
        stringResponse->prepare_payload();
        serializer.emplace(*stringResponse);

        for (auto& conn : connections)
        {
            boost::beast::http::async_write(
                conn, *serializer,
                [&](const boost::system::error_code& ec,
                    std::size_t bytes_transferred) {
                    BMCWEB_LOG_DEBUG << this << " Wrote " << bytes_transferred
                                     << " bytes";
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "Error while sending the events";
                    }
                });
        }
    }

    void handleEvents()
    {

        std::ifstream logStream(eventLogFile);
        if (!logStream.is_open())
        {
            return;
        }

        nlohmann::json logEntryArray = nlohmann::json::array();
        std::string logEvent;
        int64_t currTS{0};
        uint64_t memberId{0};

        while (std::getline(logStream, logEvent))
        {
            std::vector<std::string> logFields;
            boost::split(logFields, logEvent, boost::is_any_of(","));

            errno = 0;
            currTS = strtoll(logFields[0].c_str(), NULL, 10);

            // Not a valid timestamp
            if (errno)
            {
                continue;
            }

            // skip those entries that are already sent
            if (currTS > lastTS)
            {
                logEntryArray.push_back({});
                nlohmann::json& eventJson = logEntryArray.back();

                eventJson = {{"EventId", std::move(eventId)},
                             {"EventTimestamp", std::move(currTS)},
                             {"MemberId", std::move(memberId)},
                             {"Message", std::move(logFields[2])},
                             {"MessageId", std::move(logFields[1])},
                             {"OriginOfCondition",
                              {"@odata.id", std::move(logFields[3])}}};
            }
        }

        if (currTS > lastTS)
        {
            lastTS = currTS;
        }

        nlohmann::json msg = {
            {"@odata.context", "/redfish/v1/$metadata#Event.Event"},
            {"@odata.id", std::string("/redfish/v1/EventService/Events/") +
                              std::to_string(eventId)},
            {"@odata.type", "#Event.v1_1_0.Event"},
            {"Id", eventId},
            {"Events", logEntryArray}};
        this->sendEvents(msg.dump());
    }

    static int notificationHandler(sd_event_source* eventSource,
                                   const struct inotify_event* event,
                                   void* data)
    {
        // Not the ones we are interested in
        if (!(event->mask & eventLogFileAction))
        {
            return 0;
        }

        if (data)
        {
            auto handler = static_cast<EventHandler*>(data);
            handler->handleEvents();
        }
        return 0;
    }

    static int setup()
    {
        int rc = 0;

        sd_event* event;
        rc = sd_event_default(&event);
        if (rc < 0)
        {
            BMCWEB_LOG_ERROR << "Unable to acquire a reference to default "
                                "event loop object...";
            return rc;
        }

        eventPtr = EventPtr(event);

        // Add inotify file system inode event source and setup a
        // callback handler to be invoked on events
        rc = sd_event_add_inotify(eventPtr.get(), nullptr, eventLogFile,
                                  eventLogFileAction, notificationHandler,
                                  &EventHandler::getInstance());
        if (rc < 0)
        {
            BMCWEB_LOG_ERROR
                << "File system inode event source setup failed...";
            return rc;
        }

        return rc;
    }
};

} // namespace redfish
