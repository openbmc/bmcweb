#include "event_service_store.hpp"
#include "subscription.hpp.hpp"

#include <nlohmann/json.hpp>

#include <string_view>

#include <gtest/gtest.h>
namespace redfish
{
TEST(EventServiceManager, submitTestEVent)
{
    boost::asio::io_context io;
    boost::urls::url url;
    auto userSubscription =
        std::make_shared<persistent_data::UserSubscription>();
    {
        Subscription sub(userSubscription, url, io);
        std::optional<int64_t> eventGroupId;
        std::optional<std::string> eventId;
        std::optional<std::string> eventTimestamp;
        std::optional<std::string> message;
        std::optional<std::vector<std::string>> messageArgs;
        std::optional<std::string> messageId;
        std::optional<std::string> originOfCondition;
        std::optional<std::string> resolution;
        std::optional<std::string> severity;
        TestEvent testEvent(eventGroupId, eventId, eventTimestamp, message,
                            messageArgs, messageId, originOfCondition,
                            resolution, severity);
        EXPECT_TRUE(sub.sendTestEventLog(testEvent));
    }
    {
        Subscription sub(userSubscription, url, io);
        std::optional<int64_t> eventGroupId = 1;
        std::optional<std::string> eventId = "GPU-RST-RECOMM-EVENT";
        std::optional<std::string> eventTimestamp = "2021-01-01T00:00:00Z";
        std::optional<std::string> message = "Custom Message";
        std::optional<std::vector<std::string>> messageArgs =
            std::vector<std::string>{"GPU_SXM_1", "Restart Recommended"};
        std::optional<std::string> messageId = "Base.1.13.ResetRecommended";
        std::optional<std::string> originOfCondition =
            "/redfish/v1/Chassis/GPU_SXM_1";
        std::optional<std::string> resolution =
            "Reset the GPU at the next service window since the ECC errors are contained";
        std::optional<std::string> severity = "Informational";
        TestEvent testEvent(eventGroupId, eventId, eventTimestamp, message,
                            messageArgs, messageId, originOfCondition,
                            resolution, severity);
        EXPECT_TRUE(sub.sendTestEventLog(testEvent));
    }
    {
        Subscription sub(userSubscription, url, io);
        std::optional<int64_t> eventGroupId = 1;
        std::optional<std::string> eventId = "dummyEvent";
        std::optional<std::string> eventTimestamp = "2021-01";
        std::optional<std::string> message = "Test Message";
        std::optional<std::vector<std::string>> messageArgs =
            std::vector<std::string>{"arg1", "arg2"};
        std::optional<std::string> messageId = "Dummy message ID";
        std::optional<std::string> originOfCondition =
            "/redfish/v1/Chassis/GPU_SXM_1";
        std::optional<std::string> resolution = "custom resoultion";
        std::optional<std::string> severity = "whatever";
        TestEvent testEvent(eventGroupId, eventId, eventTimestamp, message,
                            messageArgs, messageId, originOfCondition,

                            resolution, severity);
        EXPECT_TRUE(sub.sendTestEventLog(testEvent));
    }
}
} // namespace redfish
