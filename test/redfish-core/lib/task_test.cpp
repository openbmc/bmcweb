// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "io_context_singleton.hpp"
#include "task.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

namespace redfish::task
{
namespace
{

void runPendingHandlers()
{
    getIoContext().restart();
    getIoContext().run_for(std::chrono::seconds(5));
}

std::shared_ptr<TaskData> createTaskCountingCallbacks(size_t& callbacks)
{
    return TaskData::createTask(
        [&callbacks](const boost::system::error_code& /*ec*/,
                     sdbusplus::message_t& /*msg*/,
                     const std::shared_ptr<TaskData>& /*taskData*/) {
            callbacks++;
            return completed;
        },
        "");
}

class TaskTest : public ::testing::Test
{
  protected:
    // Remove the tasks created by this test
    void TearDown() override
    {
        for (const std::shared_ptr<TaskData>& task : tasks)
        {
            task->finishTask();
            task->timer.cancel();
        }
        runPendingHandlers();
        tasks.clear();
    }
};

TEST_F(TaskTest, RunningTaskMonitorReturnsAccepted)
{
    size_t callbacks = 0;
    std::shared_ptr<TaskData> task = createTaskCountingCallbacks(callbacks);
    task->startTimer(std::chrono::seconds(300));

    crow::Response res;
    task->populateResp(res);

    EXPECT_EQ(res.result(), boost::beast::http::status::accepted);
    EXPECT_EQ(res.getHeaderValue(boost::beast::http::field::location),
              std::format("/redfish/v1/TaskService/TaskMonitors/{}",
                          task->index));
    EXPECT_EQ(res.jsonValue["TaskState"], "Running");

    task->finishTask();
    task->timer.cancel();
    runPendingHandlers();
    EXPECT_EQ(callbacks, 0U);
}

// Without a result the monitor returns 204 once, as before
TEST_F(TaskTest, TimeoutWithoutResultGivesNoContent)
{
    size_t callbacks = 0;
    std::shared_ptr<TaskData> task = createTaskCountingCallbacks(callbacks);
    task->startTimer(std::chrono::seconds(0));
    runPendingHandlers();

    EXPECT_EQ(callbacks, 1U);
    EXPECT_EQ(task->state, "Cancelled");

    crow::Response res;
    task->populateResp(res);
    EXPECT_EQ(res.result(), boost::beast::http::status::no_content);
    EXPECT_TRUE(task->gave204);
}

TEST_F(TaskTest, FinishedTaskMonitorReturnsResult)
{
    size_t callbacks = 0;
    std::shared_ptr<TaskData> task = createTaskCountingCallbacks(callbacks);
    task->startTimer(std::chrono::seconds(300));
    task->setResult(boost::beast::http::status::ok,
                    nlohmann::json{{"Version", "1.2.0"}});
    task->finishTask();
    task->timer.cancel();
    runPendingHandlers();

    crow::Response res;
    task->populateResp(res);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(res.jsonValue["Version"], "1.2.0");
    EXPECT_FALSE(task->gave204);
    EXPECT_EQ(callbacks, 0U);
}

// The timer firing after the task finished must not cancel it
TEST_F(TaskTest, ExpiryAfterFinishKeepsTask)
{
    size_t callbacks = 0;
    std::shared_ptr<TaskData> task = createTaskCountingCallbacks(callbacks);
    task->startTimer(std::chrono::seconds(0));

    task->state = "Completed";
    task->setResult(boost::beast::http::status::ok,
                    nlohmann::json{{"Version", "1.2.0"}});
    task->finishTask();
    runPendingHandlers();

    EXPECT_EQ(task->state, "Completed");
    EXPECT_EQ(callbacks, 0U);
    crow::Response res;
    task->populateResp(res);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
}

TEST_F(TaskTest, ResultMonitorCanBeReadRepeatedly)
{
    size_t callbacks = 0;
    std::shared_ptr<TaskData> task = createTaskCountingCallbacks(callbacks);
    task->setResult(boost::beast::http::status::ok,
                    nlohmann::json{{"SignedMeasurements", "c2lnbmVk"}});
    task->finishTask();
    App app;
    requestRoutesTaskMonitor(app);
    app.validate();
    std::error_code ec;
    auto req = std::make_shared<crow::Request>(
        crow::Request::Body{
            boost::beast::http::verb::get,
            std::format("/redfish/v1/TaskService/TaskMonitors/{}", task->index),
            11},
        ec);
    ASSERT_FALSE(ec);
    for (size_t poll = 0; poll < 2; ++poll)
    {
        auto response = std::make_shared<bmcweb::AsyncResp>();
        app.handle(req, response);
        EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
        EXPECT_EQ(response->res.jsonValue["SignedMeasurements"], "c2lnbmVk");
    }
}

TEST_F(TaskTest, LegacyMonitorIsGoneAfterNoContent)
{
    size_t callbacks = 0;
    std::shared_ptr<TaskData> task = createTaskCountingCallbacks(callbacks);
    task->finishTask();
    App app;
    requestRoutesTaskMonitor(app);
    app.validate();
    std::error_code ec;
    auto req = std::make_shared<crow::Request>(
        crow::Request::Body{
            boost::beast::http::verb::get,
            std::format("/redfish/v1/TaskService/TaskMonitors/{}", task->index),
            11},
        ec);
    ASSERT_FALSE(ec);
    auto first = std::make_shared<bmcweb::AsyncResp>();
    app.handle(req, first);
    EXPECT_EQ(first->res.result(), boost::beast::http::status::no_content);
    auto second = std::make_shared<bmcweb::AsyncResp>();
    app.handle(req, second);
    EXPECT_EQ(second->res.result(), boost::beast::http::status::not_found);
}

} // namespace
} // namespace redfish::task
