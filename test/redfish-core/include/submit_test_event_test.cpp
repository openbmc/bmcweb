#include "event_service_manager.hpp"
#include "subscription.hpp"

#include <boost/url/url.hpp>

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::Optional;
using ::testing::StrEq;

namespace redfish
{
static TestEvent createTestEvent()
{
    TestEvent testEvent;
    testEvent.eventGroupId = 1;
    testEvent.eventTimestamp = "2021-01";
    testEvent.message = "Test Message";
    testEvent.messageArgs = std::vector<std::string>{"arg1", "arg2"};
    testEvent.messageId = "Dummy message ID";
    testEvent.originOfCondition = "/redfish/v1/Chassis/GPU_SXM_1";
    testEvent.resolution = "custom resolution";
    testEvent.severity = "whatever";
    return testEvent;
}

TEST(EventServiceManager, submitTestEVent)
{
    boost::urls::url url;
    EventServiceManager& evt = EventServiceManager::getInstance();
    {
        TestEvent testEvent;
        EXPECT_TRUE(evt.sendTestEventLog(testEvent));
    }
    {
        TestEvent testEvent;
        testEvent.eventGroupId = 1;
        testEvent.eventTimestamp = "2021-01-01T00:00:00Z";
        testEvent.message = "Custom Message";
        testEvent.messageArgs =
            std::vector<std::string>{"GPU_SXM_1", "Restart Recommended"};
        testEvent.messageId = "Base.1.13.ResetRecommended";
        testEvent.originOfCondition = "/redfish/v1/Chassis/GPU_SXM_1";
        testEvent.resolution =
            "Reset the GPU at the next service window since the ECC errors are contained";
        testEvent.severity = "Informational";
        EXPECT_TRUE(evt.sendTestEventLog(testEvent));
    }
    {
        TestEvent testEvent = createTestEvent();

        bool result = evt.sendTestEventLog(testEvent);

        EXPECT_TRUE(result);

        EXPECT_THAT(testEvent.eventGroupId, Optional(1));
        EXPECT_THAT(testEvent.eventTimestamp, Optional(StrEq("2021-01")));
        EXPECT_THAT(testEvent.message, Optional(StrEq("Test Message")));
        EXPECT_THAT(testEvent.messageId, Optional(StrEq("Dummy message ID")));
        EXPECT_THAT(testEvent.originOfCondition,
                    Optional(StrEq("/redfish/v1/Chassis/GPU_SXM_1")));
        EXPECT_THAT(testEvent.resolution, Optional(StrEq("custom resolution")));
        EXPECT_THAT(testEvent.severity, Optional(StrEq("whatever")));
    }
}
} // namespace redfish
