#include "app.hpp"
#include "event_service_manager.hpp"
#include "event_service_store.hpp"
#include "subscription.hpp"
#include "redfish.hpp"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{
using ::testing::EndsWith;

TEST(Redfish, PathsShouldValidate)
{
    crow::App app;

    RedfishService redfish(app);

    app.validate();

    for (const std::string* route : app.getRoutes())
    {
        ASSERT_NE(route, nullptr);
        EXPECT_THAT(*route, EndsWith("/"));
    }
}

TEST(EventServiceManager, DefaultCountsAreZeroAfterReset)
{
    EventServiceManager& manager = EventServiceManager::getInstance();
    manager.resetForTest();

    EXPECT_EQ(manager.getNumberOfSubscriptions(), 0U);
    EXPECT_EQ(manager.getNumberOfEventLogSubscribers(), 0U);
    EXPECT_EQ(manager.getNumberOfMetricReportSubscribers(), 0U);
}

TEST(EventServiceManager, CountsUpdateWhenAddingSubscriptions)
{
    EventServiceManager& manager = EventServiceManager::getInstance();
    manager.resetForTest();

    persistent_data::UserSubscription eventSub;
    eventSub.destinationUrl = *boost::urls::parse_absolute_uri("http://127.0.0.1/event");
    eventSub.protocol = "Redfish";
    eventSub.retryPolicy = "TerminateAfterRetries";
    eventSub.eventFormatType = eventFormatType;
    eventSub.subscriptionType = subscriptionTypeSSE;

    persistent_data::UserSubscription metricSub;
    metricSub.destinationUrl = *boost::urls::parse_absolute_uri("http://127.0.0.1/metric");
    metricSub.protocol = "Redfish";
    metricSub.retryPolicy = "TerminateAfterRetries";
    metricSub.eventFormatType = metricReportFormatType;
    metricSub.subscriptionType = subscriptionTypeSSE;

    auto eventSubscription = std::make_shared<Subscription>(
        std::make_shared<persistent_data::UserSubscription>(eventSub),
        eventSub.destinationUrl, getIoContext());
    auto metricSubscription = std::make_shared<Subscription>(
        std::make_shared<persistent_data::UserSubscription>(metricSub),
        metricSub.destinationUrl, getIoContext());

    manager.addSSESubscription(eventSubscription, "");
    manager.addSSESubscription(metricSubscription, "");

    EXPECT_EQ(manager.getNumberOfSubscriptions(), 2U);
    EXPECT_EQ(manager.getNumberOfEventLogSubscribers(), 1U);
    EXPECT_EQ(manager.getNumberOfMetricReportSubscribers(), 1U);

    // Clean up so this test does not affect others.
    for (const auto& id : manager.getAllIDs())
    {
        manager.deleteSubscription(id);
    }
}

} // namespace
} // namespace redfish
