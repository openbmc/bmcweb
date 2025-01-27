#include "event_service_manager.hpp"
#include "subscription.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/url/url.hpp>

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
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

        EXPECT_EQ(testEvent.eventGroupId.value(), 1);
        EXPECT_EQ(testEvent.eventTimestamp.value(), "2021-01");
        EXPECT_EQ(testEvent.message.value(), "Test Message");
        EXPECT_EQ(testEvent.messageId.value(), "Dummy message ID");
        EXPECT_EQ(testEvent.originOfCondition.value(),
                  "/redfish/v1/Chassis/GPU_SXM_1");
        EXPECT_EQ(testEvent.resolution.value(), "custom resolution");
        EXPECT_EQ(testEvent.severity.value(), "whatever");
    }
}
} // namespace redfish
