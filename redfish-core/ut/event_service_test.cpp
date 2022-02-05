/*
// Copyright (c) 2021 NVIDIA Corporation
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

/*!
 * @file    event_service_test.cpp
 * @brief   Source code for event service testing.
 */

/* -------------------------------- Includes -------------------------------- */
#include <event_service_manager.hpp>

#include <gmock/gmock.h>

namespace redfish
{

class EventServiceInternalImplTest : public ::testing::Test
{
    void SetUp() override
    {
#ifdef BMCWEB_ENABLE_DEBUG
        crow::Logger::setLogLevel(crow::LogLevel::Debug);
#endif // BMCWEB_ENABLE_DEBUG
        BMCWEB_LOG_DEBUG << "Debug log enabled";
    }
};

/* --------------------------------- Tests --------------------------------- */
TEST_F(EventServiceInternalImplTest, ClassEventPosTest)
{

    // event without argument and customized fields
    nlohmann::json event1TargetLogEntry = {
        {"MessageId", "ResourceEvent.1.0.ResourceCreated"},
        {"MessageSeverity", "OK"},
        {"Message", "The resource has been created successfully."}};
    Event event1(event1TargetLogEntry["MessageId"]);
    nlohmann::json event1LogEntry;

    event1.setRegistryMsg();
    event1.formatEventLogEntry(event1LogEntry);
    EXPECT_EQ(event1TargetLogEntry, event1LogEntry);

    // event with argument(s) but no customized fields
    nlohmann::json event2TargetLogEntry = {
        {"MessageId", "ResourceEvent.1.0.ResourceErrorsDetected"},
        {"MessageSeverity", "Warning"},
        {"Message", "The resource property GPU1 PWR_GOOD status has detected "
                    "errors of type 'interrupt asserted'."},
        {"MessageArgs", std::vector<std::string>{"GPU1 PWR_GOOD status",
                                                 "interrupt asserted"}}};
    Event event2(event2TargetLogEntry["MessageId"]);
    nlohmann::json event2LogEntry;

    EXPECT_EQ(event2.setRegistryMsg(event2TargetLogEntry["MessageArgs"]), 0);
    event2.formatEventLogEntry(event2LogEntry);
    EXPECT_EQ(event2TargetLogEntry, event2LogEntry);

    // event with customized message and severity
    nlohmann::json event3TargetLogEntry = {
        {"MessageId", "ResourceEvent.1.0.ResourceCreated"},
        {"MessageSeverity", "Warning"},
        {"Message", "The resource GPU0 has been created successfully."},
        {"MessageArgs", std::vector<std::string>{"GPU0"}}};
    Event event3(event3TargetLogEntry["MessageId"]);
    nlohmann::json event3LogEntry;

    event3.messageSeverity = event3TargetLogEntry["MessageSeverity"];
    EXPECT_EQ(
        event3.setCustomMsg("The resource %1 has been created successfully.",
                            event3TargetLogEntry["MessageArgs"]),
        0);
    event3.formatEventLogEntry(event3LogEntry);
    EXPECT_EQ(event3TargetLogEntry, event3LogEntry);
}

TEST_F(EventServiceInternalImplTest, ClassEventNegaTest)
{
    // event with incorrect number of argument(s) but no customized message
    nlohmann::json event1LogEntry = {
        {"MessageId", "ResourceEvent.1.0.ResourceCreated"},
        {"MessageSeverity", "OK"},
        {"Message", "The resource has been created successfully."},
        {"MessageArgs", std::vector<std::string>{"Dummy"}}};
    Event event1(event1LogEntry["MessageId"]);

    EXPECT_NE(event1.setRegistryMsg(event1LogEntry["MessageArgs"]), 0);

    // event with incorrect number of argument(s) and customized message
    nlohmann::json event2LogEntry = {
        {"MessageId", "ResourceEvent.1.0.ResourceCreated"},
        {"MessageSeverity", "OK"},
        {"Message", "The resource %1 has been created successfully."},
        {"MessageArgs", std::vector<std::string>{}}};
    Event event2(event2LogEntry["MessageId"]);

    EXPECT_NE(event2.setCustomMsg(event2LogEntry["Message"],
                                  event2LogEntry["MessageArgs"]),
              0);

    // event constructed with invalid MessageId
    Event event3("aaa");
    EXPECT_FALSE(event3.isValid());
}

} // namespace redfish