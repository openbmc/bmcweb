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

#include "app.hpp"
#include "dbus_utility.hpp"
#include "event_service_manager.hpp"
#include "health.hpp"
#include "http/parsing.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "task_messages.hpp"

#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>
#include <memory>
#include <variant>

namespace redfish
{

namespace task
{
constexpr size_t maxTaskCount = 100; // arbitrary limit

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::deque<std::shared_ptr<struct TaskData>> tasks;

constexpr bool completed = true;

struct Payload
{
    explicit Payload(const crow::Request& req) :
        targetUri(req.url().encoded_path()), httpOperation(req.method())
    {
        using field_ns = boost::beast::http::field;
        constexpr const std::array<boost::beast::http::field, 7>
            headerWhitelist = {field_ns::accept,     field_ns::accept_encoding,
                               field_ns::user_agent, field_ns::host,
                               field_ns::connection, field_ns::content_length,
                               field_ns::upgrade};

        JsonParseResult ret = parseRequestAsJson(req, jsonBody);
        if (ret != JsonParseResult::Success)
        {
            return;
        }

        for (const auto& field : req.fields())
        {
            if (std::find(headerWhitelist.begin(), headerWhitelist.end(),
                          field.name()) == headerWhitelist.end())
            {
                continue;
            }
            std::string header;
            header.reserve(field.name_string().size() + 2 +
                           field.value().size());
            header += field.name_string();
            header += ": ";
            header += field.value();
            httpHeaders.emplace_back(std::move(header));
        }
    }
    Payload() = delete;

    boost::urls::url targetUri;
    boost::beast::http::verb httpOperation;
    std::vector<std::string> httpHeaders;
    nlohmann::json jsonBody;
};

struct TaskData : std::enable_shared_from_this<TaskData>
{
  private:
    TaskData(
        std::function<bool(boost::system::error_code, sdbusplus::message_t&,
                           const std::shared_ptr<TaskData>&)>&& handler,
        const std::string& matchIn, size_t idx) :
        callback(std::move(handler)),
        matchStr(matchIn), index(idx),
        startTime(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now())),
        status("OK"), state("Running"), messages(nlohmann::json::array()),
        timer(crow::connections::systemBus->get_io_context())

    {}

  public:
    TaskData() = delete;

    static std::shared_ptr<TaskData>& createTask(
        std::function<bool(boost::system::error_code, sdbusplus::message_t&,
                           const std::shared_ptr<TaskData>&)>&& handler,
        const std::string& match)
    {
        static size_t lastTask = 0;
        struct MakeSharedHelper : public TaskData
        {
            MakeSharedHelper(
                std::function<bool(boost::system::error_code,
                                   sdbusplus::message_t&,
                                   const std::shared_ptr<TaskData>&)>&& handler,
                const std::string& match2, size_t idx) :
                TaskData(std::move(handler), match2, idx)
            {}
        };

        if (tasks.size() >= maxTaskCount)
        {
            const auto& last = tasks.front();

            // destroy all references
            last->timer.cancel();
            last->match.reset();
            tasks.pop_front();
        }

        return tasks.emplace_back(std::make_shared<MakeSharedHelper>(
            std::move(handler), match, lastTask++));
    }

    void populateResp(crow::Response& res, size_t retryAfterSeconds = 30)
    {
        if (!endTime)
        {
            res.result(boost::beast::http::status::accepted);
            std::string strIdx = std::to_string(index);
            std::string uri = "/redfish/v1/TaskService/Tasks/" + strIdx;

            res.jsonValue["@odata.id"] = uri;
            res.jsonValue["@odata.type"] = "#Task.v1_4_3.Task";
            res.jsonValue["Id"] = strIdx;
            res.jsonValue["TaskState"] = state;
            res.jsonValue["TaskStatus"] = status;

            res.addHeader(boost::beast::http::field::location,
                          uri + "/Monitor");
            res.addHeader(boost::beast::http::field::retry_after,
                          std::to_string(retryAfterSeconds));
        }
        else if (!gave204)
        {
            res.result(boost::beast::http::status::no_content);
            gave204 = true;
        }
    }

    void finishTask()
    {
        endTime = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
    }

    void extendTimer(const std::chrono::seconds& timeout)
    {
        timer.expires_after(timeout);
        timer.async_wait(
            [self = shared_from_this()](boost::system::error_code ec) {
            if (ec == boost::asio::error::operation_aborted)
            {
                return; // completed successfully
            }
            if (!ec)
            {
                // change ec to error as timer expired
                ec = boost::asio::error::operation_aborted;
            }
            self->match.reset();
            sdbusplus::message_t msg;
            self->finishTask();
            self->state = "Cancelled";
            self->status = "Warning";
            self->messages.emplace_back(
                messages::taskAborted(std::to_string(self->index)));
            // Send event :TaskAborted
            self->sendTaskEvent(self->state, self->index);
            self->callback(ec, msg, self);
        });
    }

    static void sendTaskEvent(std::string_view state, size_t index)
    {
        std::string origin =
            "/redfish/v1/TaskService/Tasks/" + std::to_string(index);
        std::string resType = "Task";
        // TaskState enums which should send out an event are:
        // "Starting" = taskResumed
        // "Running" = taskStarted
        // "Suspended" = taskPaused
        // "Interrupted" = taskPaused
        // "Pending" = taskPaused
        // "Stopping" = taskAborted
        // "Completed" = taskCompletedOK
        // "Killed" = taskRemoved
        // "Exception" = taskCompletedWarning
        // "Cancelled" = taskCancelled
        if (state == "Starting")
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskResumed(std::to_string(index)), origin,
                resType);
        }
        else if (state == "Running")
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskStarted(std::to_string(index)), origin,
                resType);
        }
        else if ((state == "Suspended") || (state == "Interrupted") ||
                 (state == "Pending"))
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskPaused(std::to_string(index)), origin,
                resType);
        }
        else if (state == "Stopping")
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskAborted(std::to_string(index)), origin,
                resType);
        }
        else if (state == "Completed")
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskCompletedOK(std::to_string(index)),
                origin, resType);
        }
        else if (state == "Killed")
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskRemoved(std::to_string(index)), origin,
                resType);
        }
        else if (state == "Exception")
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskCompletedWarning(std::to_string(index)),
                origin, resType);
        }
        else if (state == "Cancelled")
        {
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::taskCancelled(std::to_string(index)), origin,
                resType);
        }
        else
        {
            BMCWEB_LOG_INFO << "sendTaskEvent: No events to send";
        }
    }

    void startTimer(const std::chrono::seconds& timeout)
    {
        if (match)
        {
            return;
        }
        match = std::make_unique<sdbusplus::bus::match_t>(
            static_cast<sdbusplus::bus_t&>(*crow::connections::systemBus),
            matchStr,
            [self = shared_from_this()](sdbusplus::message_t& message) {
            boost::system::error_code ec;

            // callback to return True if callback is done, callback needs
            // to update status itself if needed
            if (self->callback(ec, message, self) == task::completed)
            {
                self->timer.cancel();
                self->finishTask();

                // Send event
                self->sendTaskEvent(self->state, self->index);

                // reset the match after the callback was successful
                boost::asio::post(
                    crow::connections::systemBus->get_io_context(),
                    [self] { self->match.reset(); });
                return;
            }
            });

        extendTimer(timeout);
        messages.emplace_back(messages::taskStarted(std::to_string(index)));
        // Send event : TaskStarted
        sendTaskEvent(state, index);
    }

    std::function<bool(boost::system::error_code, sdbusplus::message_t&,
                       const std::shared_ptr<TaskData>&)>
        callback;
    std::string matchStr;
    size_t index;
    time_t startTime;
    std::string status;
    std::string state;
    nlohmann::json messages;
    boost::asio::steady_timer timer;
    std::unique_ptr<sdbusplus::bus::match_t> match;
    std::optional<time_t> endTime;
    std::optional<Payload> payload;
    bool gave204 = false;
    int percentComplete = 0;
};

} // namespace task

inline void requestRoutesTaskMonitor(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/Tasks/<str>/Monitor/")
        .privileges(redfish::privileges::getTask)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& strParam) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        auto find = std::find_if(
            task::tasks.begin(), task::tasks.end(),
            [&strParam](const std::shared_ptr<task::TaskData>& task) {
            if (!task)
            {
                return false;
            }

            // we compare against the string version as on failure
            // strtoul returns 0
            return std::to_string(task->index) == strParam;
            });

        if (find == task::tasks.end())
        {
            messages::resourceNotFound(asyncResp->res, "Task", strParam);
            return;
        }
        std::shared_ptr<task::TaskData>& ptr = *find;
        // monitor expires after 204
        if (ptr->gave204)
        {
            messages::resourceNotFound(asyncResp->res, "Task", strParam);
            return;
        }
        ptr->populateResp(asyncResp->res);
        });
}

inline void requestRoutesTask(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/Tasks/<str>/")
        .privileges(redfish::privileges::getTask)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& strParam) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        auto find = std::find_if(
            task::tasks.begin(), task::tasks.end(),
            [&strParam](const std::shared_ptr<task::TaskData>& task) {
            if (!task)
            {
                return false;
            }

            // we compare against the string version as on failure
            // strtoul returns 0
            return std::to_string(task->index) == strParam;
            });

        if (find == task::tasks.end())
        {
            messages::resourceNotFound(asyncResp->res, "Task", strParam);
            return;
        }

        const std::shared_ptr<task::TaskData>& ptr = *find;

        asyncResp->res.jsonValue["@odata.type"] = "#Task.v1_4_3.Task";
        asyncResp->res.jsonValue["Id"] = strParam;
        asyncResp->res.jsonValue["Name"] = "Task " + strParam;
        asyncResp->res.jsonValue["TaskState"] = ptr->state;
        asyncResp->res.jsonValue["StartTime"] =
            redfish::time_utils::getDateTimeStdtime(ptr->startTime);
        if (ptr->endTime)
        {
            asyncResp->res.jsonValue["EndTime"] =
                redfish::time_utils::getDateTimeStdtime(*(ptr->endTime));
        }
        asyncResp->res.jsonValue["TaskStatus"] = ptr->status;
        asyncResp->res.jsonValue["Messages"] = ptr->messages;
        asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
            "redfish", "v1", "TaskService", "Tasks", strParam);
        if (!ptr->gave204)
        {
            asyncResp->res.jsonValue["TaskMonitor"] =
                "/redfish/v1/TaskService/Tasks/" + strParam + "/Monitor";
        }
        if (ptr->payload)
        {
            const task::Payload& p = *(ptr->payload);
            asyncResp->res.jsonValue["Payload"]["TargetUri"] = p.targetUri;
            asyncResp->res.jsonValue["Payload"]["HttpOperation"] =
                boost::beast::http::to_string(p.httpOperation);
            asyncResp->res.jsonValue["Payload"]["HttpHeaders"] = p.httpHeaders;
            asyncResp->res.jsonValue["Payload"]["JsonBody"] = p.jsonBody.dump(
                2, ' ', true, nlohmann::json::error_handler_t::replace);
        }
        asyncResp->res.jsonValue["PercentComplete"] = ptr->percentComplete;
        });
}

inline void requestRoutesTaskCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/Tasks/")
        .privileges(redfish::privileges::getTaskCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#TaskCollection.TaskCollection";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/TaskService/Tasks";
        asyncResp->res.jsonValue["Name"] = "Task Collection";
        asyncResp->res.jsonValue["Members@odata.count"] = task::tasks.size();
        nlohmann::json& members = asyncResp->res.jsonValue["Members"];
        members = nlohmann::json::array();

        for (const std::shared_ptr<task::TaskData>& task : task::tasks)
        {
            if (task == nullptr)
            {
                continue; // shouldn't be possible
            }
            nlohmann::json::object_t member;
            member["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "TaskService", "Tasks",
                std::to_string(task->index));
            members.emplace_back(std::move(member));
        }
        });
}

inline void requestRoutesTaskService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TaskService/")
        .privileges(redfish::privileges::getTaskService)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!redfish::setUpRedfishRoute(app, req, asyncResp))
        {
            return;
        }
        asyncResp->res.jsonValue["@odata.type"] =
            "#TaskService.v1_1_4.TaskService";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/TaskService";
        asyncResp->res.jsonValue["Name"] = "Task Service";
        asyncResp->res.jsonValue["Id"] = "TaskService";
        asyncResp->res.jsonValue["DateTime"] =
            redfish::time_utils::getDateTimeOffsetNow().first;
        asyncResp->res.jsonValue["CompletedTaskOverWritePolicy"] = "Oldest";

        asyncResp->res.jsonValue["LifeCycleEventOnTaskStateChange"] = true;

        auto health = std::make_shared<HealthPopulate>(asyncResp);
        health->populate();
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        asyncResp->res.jsonValue["ServiceEnabled"] = true;
        asyncResp->res.jsonValue["Tasks"]["@odata.id"] =
            "/redfish/v1/TaskService/Tasks";
        });
}

} // namespace redfish
