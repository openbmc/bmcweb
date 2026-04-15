#pragma once

#include "http_request.hpp"
#include "task_messages.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>

namespace redfish
{
namespace task
{

constexpr size_t maxTaskCount = 100; // arbitrary limit

constexpr bool completed = true;
} // namespace task

struct Payload
{
    explicit Payload(const crow::Request& req) :
        targetUri(req.url().encoded_path()), httpOperation(req.methodString())
    {
        using field_ns = boost::beast::http::field;
        constexpr const std::array<boost::beast::http::field, 7>
            headerWhitelist = {field_ns::accept,     field_ns::accept_encoding,
                               field_ns::user_agent, field_ns::host,
                               field_ns::connection, field_ns::content_length,
                               field_ns::upgrade};

        nlohmann::json jsonBody;
        JsonParseResult ret = parseRequestAsJson(req, jsonBody);
        if (ret != JsonParseResult::Success)
        {
            return;
        }
        using error_handler_t = nlohmann::json::error_handler_t;
        jsonBodyStr = jsonBody.dump(-1, ' ', true, error_handler_t::replace);

        for (const auto& field : req.fields())
        {
            if (!std::ranges::contains(headerWhitelist, field.name()))
            {
                continue;
            }
            std::string header;
            header.reserve(
                field.name_string().size() + 2 + field.value().size());
            header += field.name_string();
            header += ": ";
            header += field.value();
            httpHeaders.emplace_back(std::move(header));
        }
    }
    Payload() = delete;

    std::string targetUri;
    std::string httpOperation;
    nlohmann::json::array_t httpHeaders;
    std::string jsonBodyStr;
};

struct TaskData : std::enable_shared_from_this<TaskData>
{
  private:
    TaskData(std::move_only_function<
                 bool(const boost::system::error_code&, sdbusplus::message_t&,
                      const std::shared_ptr<TaskData>&)>&& handler,
             const std::string& matchIn, size_t idx) :
        callback(std::move(handler)), matchStr(matchIn), index(idx),
        startTime(std::chrono::system_clock::now()), status("OK"),
        state("Running"), timer(crow::connections::systemBus->get_io_context())

    {}

  public:
    // TODO(ed) this really needs to be split into a "task manager" class
    inline static std::deque<std::shared_ptr<TaskData>> tasks;

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

        if (tasks.size() >= task::maxTaskCount)
        {
            const auto last = getTaskToRemove();

            // destroy all references
            (*last)->timer.cancel();
            (*last)->match.reset();
            tasks.erase(last);
        }

        return tasks.emplace_back(std::make_shared<MakeSharedHelper>(
            std::move(handler), match, lastTask++));
    }

    /**
     * @brief Get the first completed/aborted task or oldest running task to
     * remove
     */
    static std::deque<std::shared_ptr<TaskData>>::iterator getTaskToRemove()
    {
        static constexpr std::array<std::string_view, 5> activeStates = {
            "Running", "Pending", "Starting", "Suspended", "Interrupted"};

        auto it = std::ranges::find_if(tasks, [](const auto& task) {
            return std::ranges::find(activeStates, task->state) ==
                   activeStates.end();
        });

        return (it != tasks.end()) ? it : tasks.begin();
    }

    void populateResp(crow::Response& res, size_t retryAfterSeconds = 30)
    {
        if (!endTime)
        {
            res.result(boost::beast::http::status::accepted);
            std::string strIdx = std::to_string(index);
            boost::urls::url uri =
                boost::urls::format("/redfish/v1/TaskService/Tasks/{}", strIdx);

            res.jsonValue["@odata.id"] = uri;
            res.jsonValue["@odata.type"] = "#Task.v1_4_3.Task";
            res.jsonValue["Id"] = strIdx;
            res.jsonValue["TaskState"] = state;
            res.jsonValue["TaskStatus"] = status;

            boost::urls::url taskMonitor = boost::urls::format(
                "/redfish/v1/TaskService/TaskMonitors/{}", strIdx);

            res.addHeader(boost::beast::http::field::location,
                          taskMonitor.buffer());
            res.addHeader(boost::beast::http::field::retry_after,
                          std::to_string(retryAfterSeconds));
            res.jsonValue["Name"] = "Task " + strIdx;
            res.jsonValue["StartTime"] =
                redfish::time_utils::getDateTimeStdtime(startTime);
            res.jsonValue["Messages"] = messages;
            res.jsonValue["TaskMonitor"] = taskMonitor;
            res.jsonValue["HidePayload"] = !payload;
            if (payload)
            {
                const Payload& p = *payload;
                nlohmann::json::object_t payloadObj;
                payloadObj["TargetUri"] = p.targetUri;
                payloadObj["HttpOperation"] = p.httpOperation;
                payloadObj["HttpHeaders"] = p.httpHeaders;
                if (!p.jsonBodyStr.empty())
                {
                    payloadObj["JsonBody"] = p.jsonBodyStr;
                }
                res.jsonValue["Payload"] = std::move(payloadObj);
            }
            res.jsonValue["PercentComplete"] = percentComplete;
        }
        else if (!gave204)
        {
            res.result(boost::beast::http::status::no_content);
            gave204 = true;
        }
    }

    void finishTask()
    {
        endTime = std::chrono::system_clock::now();
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
                sendTaskEvent(self->state, self->index);
                self->callback(ec, msg, self);
            });
    }

    static void sendTaskEvent(std::string_view state, size_t index)
    {
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
        nlohmann::json::object_t event;
        std::string indexStr = std::to_string(index);
        if (state == "Starting")
        {
            event = redfish::messages::taskResumed(indexStr);
        }
        else if (state == "Running")
        {
            event = redfish::messages::taskStarted(indexStr);
        }
        else if ((state == "Suspended") || (state == "Interrupted") ||
                 (state == "Pending"))
        {
            event = redfish::messages::taskPaused(indexStr);
        }
        else if (state == "Stopping")
        {
            event = redfish::messages::taskAborted(indexStr);
        }
        else if (state == "Completed")
        {
            event = redfish::messages::taskCompletedOK(indexStr);
        }
        else if (state == "Killed")
        {
            event = redfish::messages::taskRemoved(indexStr);
        }
        else if (state == "Exception")
        {
            event = redfish::messages::taskCompletedWarning(indexStr);
        }
        else if (state == "Cancelled")
        {
            event = redfish::messages::taskCancelled(indexStr);
        }
        else
        {
            BMCWEB_LOG_INFO("sendTaskEvent: No events to send");
            return;
        }
        boost::urls::url origin =
            boost::urls::format("/redfish/v1/TaskService/Tasks/{}", index);
        EventServiceManager::getInstance().sendEvent(event, origin.buffer(),
                                                     "Task");
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
                    sendTaskEvent(self->state, self->index);

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

    std::move_only_function<bool(const boost::system::error_code&,
                                 sdbusplus::message_t&,
                                 const std::shared_ptr<TaskData>&)>
        callback;
    std::string matchStr;
    size_t index;
    std::chrono::system_clock::time_point startTime;
    std::string status;
    std::string state;
    nlohmann::json::array_t messages;
    boost::asio::steady_timer timer;
    std::unique_ptr<sdbusplus::bus::match_t> match;
    std::optional<std::chrono::system_clock::time_point> endTime;
    std::optional<Payload> payload;
    bool gave204 = false;
    int percentComplete = 0;
};

} // namespace redfish
