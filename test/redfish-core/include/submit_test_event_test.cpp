#include "event_service_manager.hpp"
#include "subscription.hpp"

#include <boost/url/url.hpp>

#include <array>
#include <map>
#include <memory>
#include <string>
#include <string_view>
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

class ErasingSseConnection final : public crow::sse_socket::Connection
{
  public:
    EventServiceManager* manager = nullptr;
    bool eraseOnSend = false;
    int sends = 0;

    void close(std::string_view /*msg*/) override {}

    void sendSseEvent(std::string_view /*id*/,
                      std::string_view /*msg*/) override
    {
        ++sends;
        if (eraseOnSend)
        {
            manager->deleteSseSubscription(*this);
        }
    }
};

TEST(EventServiceManager, DispatchSurvivesMidSendErase)
{
    EventServiceManager& evt = EventServiceManager::getInstance();
    auto& store = persistent_data::EventServiceStore::getInstance();
    const bool previousEnabled = store.getEventServiceConfig().enabled;
    store.getEventServiceConfig().enabled = true;

    std::array<std::unique_ptr<ErasingSseConnection>, 3> connections;
    std::map<std::string, ErasingSseConnection*> byId;
    for (auto& connection : connections)
    {
        connection = std::make_unique<ErasingSseConnection>();
        connection->manager = &evt;
        auto subscription = std::make_shared<Subscription>(*connection);
        subscription->userSub->subscriptionType = subscriptionTypeSSE;
        std::string id = evt.addSSESubscription(subscription, "");
        ASSERT_FALSE(id.empty());
        byId.emplace(id, connection.get());
    }
    ASSERT_EQ(byId.size(), 3U);
    byId.begin()->second->eraseOnSend = true;

    TestEvent event;
    event.message = "mid-dispatch erase";
    EXPECT_TRUE(evt.sendTestEventLog(event));
    EXPECT_EQ(evt.getNumberOfSubscriptions(), 2U);
    for (const auto& [id, connection] : byId)
    {
        EXPECT_EQ(connection->sends, 1) << id;
    }

    for (const auto& connection : connections)
    {
        evt.deleteSseSubscription(*connection);
    }
    // Also exercise sendEvent, sendEventsToSubs, and sendTelemetryReportToSubs
    // when the first subscriber erases itself mid-dispatch.
    auto runWithThreeSubscribers = [&](const auto& dispatchFn) {
        std::array<std::unique_ptr<ErasingSseConnection>, 3> conns;
        std::map<std::string, ErasingSseConnection*> ordered;
        for (auto& conn : conns)
        {
            conn = std::make_unique<ErasingSseConnection>();
            conn->manager = &evt;
            auto sub = std::make_shared<Subscription>(*conn);
            sub->userSub->subscriptionType = subscriptionTypeSSE;
            std::string id = evt.addSSESubscription(sub, "");
            ASSERT_FALSE(id.empty());
            ordered.emplace(id, conn.get());
        }
        ASSERT_EQ(ordered.size(), 3U);
        ordered.begin()->second->eraseOnSend = true;

        dispatchFn();

        EXPECT_EQ(evt.getNumberOfSubscriptions(), 2U);
        for (const auto& [id, conn] : ordered)
        {
            EXPECT_EQ(conn->sends, 1) << id;
        }
        for (const auto& conn : conns)
        {
            evt.deleteSseSubscription(*conn);
        }
    };

    runWithThreeSubscribers([&]() {
        nlohmann::json::object_t msg;
        msg["MessageId"] = "Base.1.13.ResetRecommended";
        evt.sendEvent(msg, "/redfish/v1/Chassis/1", "Chassis");
    });

    runWithThreeSubscribers([&]() {
        std::vector<EventLogObjectsType> records{
            EventLogObjectsType{"1", "2026-11-17T00:00:00Z",
                                "OpenBMC.0.1.PowerButtonPressed", {}}};
        EventServiceManager::sendEventsToSubs(records);
    });

    runWithThreeSubscribers([&]() {
        telemetry::TimestampReadings readings;
        EventServiceManager::sendTelemetryReportToSubs("Report1", readings);
    });
    store.getEventServiceConfig().enabled = previousEnabled;

}
} // namespace redfish
