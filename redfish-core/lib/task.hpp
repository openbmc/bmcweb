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

#include <boost/asio.hpp>
#include <boost/container/flat_map.hpp>
#include <task_messages.hpp>

#include <chrono>
#include <variant>

namespace redfish
{

namespace task
{
constexpr size_t maxTaskCount = 100; // arbitrary limit

static std::deque<std::shared_ptr<struct TaskData>> tasks;

constexpr bool completed = true;

struct Payload
{
    Payload(const crow::Request& req) :
        targetUri(req.url), httpOperation(req.methodString()),
        httpHeaders(nlohmann::json::array())

    {
        using field_ns = boost::beast::http::field;
        constexpr const std::array<boost::beast::http::field, 7>
            headerWhitelist = {field_ns::accept,     field_ns::accept_encoding,
                               field_ns::user_agent, field_ns::host,
                               field_ns::connection, field_ns::content_length,
                               field_ns::upgrade};

        jsonBody = nlohmann::json::parse(req.body, nullptr, false);
        if (jsonBody.is_discarded())
        {
            jsonBody = nullptr;
        }

        for (const auto& field : req.fields)
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

    std::string targetUri;
    std::string httpOperation;
    nlohmann::json httpHeaders;
    nlohmann::json jsonBody;
};

inline void to_json(nlohmann::json& j, const Payload& p)
{
    j = {{"TargetUri", p.targetUri},
         {"HttpOperation", p.httpOperation},
         {"HttpHeaders", p.httpHeaders},
         {"JsonBody", p.jsonBody.dump()}};
}

struct TaskData : std::enable_shared_from_this<TaskData>
{
  private:
    TaskData(std::function<bool(boost::system::error_code,
                                sdbusplus::message::message&,
                                const std::shared_ptr<TaskData>&)>&& handler,
             const std::string& match, size_t idx) :
        callback(std::move(handler)),
        matchStr(match), index(idx),
        startTime(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now())),
        status("OK"), state("Running"), messages(nlohmann::json::array()),
        timer(crow::connections::systemBus->get_io_context())

    {}
    TaskData() = delete;

  public:
    static std::shared_ptr<TaskData>& createTask(
        std::function<bool(boost::system::error_code,
                           sdbusplus::message::message&,
                           const std::shared_ptr<TaskData>&)>&& handler,
        const std::string& match)
    {
        static size_t lastTask = 0;
        struct MakeSharedHelper : public TaskData
        {
            MakeSharedHelper(
                std::function<bool(boost::system::error_code,
                                   sdbusplus::message::message&,
                                   const std::shared_ptr<TaskData>&)>&& handler,
                const std::string& match, size_t idx) :
                TaskData(std::move(handler), match, idx)
            {}
        };

        if (tasks.size() >= maxTaskCount)
        {
            auto& last = tasks.front();

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
            res.jsonValue = {{"@odata.id", uri},
                             {"@odata.type", "#Task.v1_4_3.Task"},
                             {"Id", strIdx},
                             {"TaskState", state},
                             {"TaskStatus", status}};
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

    void finishTask(void)
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
                    return; // completed succesfully
                }
                if (!ec)
                {
                    // change ec to error as timer expired
                    ec = boost::asio::error::operation_aborted;
                }
                self->match.reset();
                sdbusplus::message::message msg;
                self->finishTask();
                self->state = "Cancelled";
                self->status = "Warning";
                self->messages.emplace_back(
                    messages::taskAborted(std::to_string(self->index)));
                self->callback(ec, msg, self);
            });
    }

    void startTimer(const std::chrono::seconds& timeout)
    {
        if (match)
        {
            return;
        }
        match = std::make_unique<sdbusplus::bus::match::match>(
            static_cast<sdbusplus::bus::bus&>(*crow::connections::systemBus),
            matchStr,
            [self = shared_from_this()](sdbusplus::message::message& message) {
                boost::system::error_code ec;

                // callback to return True if callback is done, callback needs
                // to update status itself if needed
                if (self->callback(ec, message, self) == task::completed)
                {
                    self->timer.cancel();
                    self->finishTask();

                    // reset the match after the callback was successful
                    boost::asio::post(
                        crow::connections::systemBus->get_io_context(),
                        [self] { self->match.reset(); });
                    return;
                }
            });

        extendTimer(timeout);
        messages.emplace_back(messages::taskStarted(std::to_string(index)));
    }

    std::function<bool(boost::system::error_code, sdbusplus::message::message&,
                       const std::shared_ptr<TaskData>&)>
        callback;
    std::string matchStr;
    size_t index;
    time_t startTime;
    std::string status;
    std::string state;
    nlohmann::json messages;
    boost::asio::steady_timer timer;
    std::unique_ptr<sdbusplus::bus::match::match> match;
    std::optional<time_t> endTime;
    std::optional<Payload> payload;
    bool gave204 = false;
};

} // namespace task

class TaskMonitor : public Node
{
  public:
    TaskMonitor(CrowApp& app) :
        Node((app), "/redfish/v1/TaskService/Tasks/<str>/Monitor/",
             std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& strParam = params[0];
        auto find = std::find_if(
            task::tasks.begin(), task::tasks.end(),
            [&strParam](const std::shared_ptr<task::TaskData>& task) {
                if (!task)
                {
                    return false;
                }

                // we compare against the string version as on failure strtoul
                // returns 0
                return std::to_string(task->index) == strParam;
            });

        if (find == task::tasks.end())
        {
            messages::resourceNotFound(asyncResp->res, "Monitor", strParam);
            return;
        }
        std::shared_ptr<task::TaskData>& ptr = *find;
        // monitor expires after 204
        if (ptr->gave204)
        {
            messages::resourceNotFound(asyncResp->res, "Monitor", strParam);
            return;
        }
        ptr->populateResp(asyncResp->res);
    }
};

class Task : public Node
{
  public:
    Task(CrowApp& app) :
        Node((app), "/redfish/v1/TaskService/Tasks/<str>/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& strParam = params[0];
        auto find = std::find_if(
            task::tasks.begin(), task::tasks.end(),
            [&strParam](const std::shared_ptr<task::TaskData>& task) {
                if (!task)
                {
                    return false;
                }

                // we compare against the string version as on failure strtoul
                // returns 0
                return std::to_string(task->index) == strParam;
            });

        if (find == task::tasks.end())
        {
            messages::resourceNotFound(asyncResp->res, "Tasks", strParam);
            return;
        }

        std::shared_ptr<task::TaskData>& ptr = *find;

        asyncResp->res.jsonValue["@odata.type"] = "#Task.v1_4_3.Task";
        asyncResp->res.jsonValue["Id"] = strParam;
        asyncResp->res.jsonValue["Name"] = "Task " + strParam;
        asyncResp->res.jsonValue["TaskState"] = ptr->state;
        asyncResp->res.jsonValue["StartTime"] =
            crow::utility::getDateTime(ptr->startTime);
        if (ptr->endTime)
        {
            asyncResp->res.jsonValue["EndTime"] =
                crow::utility::getDateTime(*(ptr->endTime));
        }
        asyncResp->res.jsonValue["TaskStatus"] = ptr->status;
        asyncResp->res.jsonValue["Messages"] = ptr->messages;
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/TaskService/Tasks/" + strParam;
        if (!ptr->gave204)
        {
            asyncResp->res.jsonValue["TaskMonitor"] =
                "/redfish/v1/TaskService/Tasks/" + strParam + "/Monitor";
        }
        if (ptr->payload)
        {
            asyncResp->res.jsonValue["Payload"] = *(ptr->payload);
        }
    }
};

class TaskCollection : public Node
{
  public:
    TaskCollection(CrowApp& app) : Node(app, "/redfish/v1/TaskService/Tasks/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
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
            members.emplace_back(
                nlohmann::json{{"@odata.id", "/redfish/v1/TaskService/Tasks/" +
                                                 std::to_string(task->index)}});
        }
    }
};

class TaskService : public Node
{
  public:
    TaskService(CrowApp& app) : Node(app, "/redfish/v1/TaskService/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue["@odata.type"] =
            "#TaskService.v1_1_4.TaskService";
        asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/TaskService";
        asyncResp->res.jsonValue["Name"] = "Task Service";
        asyncResp->res.jsonValue["Id"] = "TaskService";
        asyncResp->res.jsonValue["DateTime"] = crow::utility::dateTimeNow();
        asyncResp->res.jsonValue["CompletedTaskOverWritePolicy"] = "Oldest";

        // todo: if we enable events, change this to true
        asyncResp->res.jsonValue["LifeCycleEventOnTaskStateChange"] = false;

        auto health = std::make_shared<HealthPopulate>(asyncResp);
        health->populate();
        asyncResp->res.jsonValue["Status"]["State"] = "Enabled";
        asyncResp->res.jsonValue["ServiceEnabled"] = true;
        asyncResp->res.jsonValue["Tasks"] = {
            {"@odata.id", "/redfish/v1/TaskService/Tasks"}};
    }
};

} // namespace redfish
