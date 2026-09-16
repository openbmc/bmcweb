// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "task.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/message.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish::task
{
namespace
{

std::shared_ptr<TaskData> addTask(const std::string& state)
{
    std::shared_ptr<TaskData> task = TaskData::createTask(
        [](const boost::system::error_code&, sdbusplus::message_t&,
           const std::shared_ptr<TaskData>&) { return true; },
        "");
    task->state = state;
    return task;
}

class GetTaskToRemove : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        tasks.clear();
    }
    void TearDown() override
    {
        tasks.clear();
    }
};

TEST_F(GetTaskToRemove, OldestTaskIsRemovedWhenAllAreActive)
{
    std::shared_ptr<TaskData> oldest = addTask("Running");
    addTask("Pending");
    addTask("Starting");
    addTask("Suspended");
    addTask("Interrupted");

    EXPECT_EQ(*TaskData::getTaskToRemove(), oldest);
}

TEST_F(GetTaskToRemove, InactiveTaskIsRemovedBeforeActiveTasks)
{
    addTask("Running");
    std::shared_ptr<TaskData> completed = addTask("Completed");

    EXPECT_EQ(*TaskData::getTaskToRemove(), completed);
}

TEST_F(GetTaskToRemove, FirstInactiveTaskIsRemoved)
{
    addTask("Running");
    std::shared_ptr<TaskData> cancelled = addTask("Cancelled");
    addTask("Exception");

    EXPECT_EQ(*TaskData::getTaskToRemove(), cancelled);
}

} // namespace
} // namespace redfish::task
