#include "bmcweb_config.h"

#include "app.hpp"
#include "event_service_manager.hpp"
#include "event_service_store.hpp"
#include "io_context_singleton.hpp"
#include "redfish.hpp"
#include "subscription.hpp"

#include <boost/url/parse.hpp>

#include <memory>
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

TEST(EventServiceManager, DefaultCountsAreZero)
{
    EventServiceManager& manager = EventServiceManager::getInstance();

    EXPECT_EQ(manager.getNumberOfSubscriptions(), 0U);
    EXPECT_EQ(manager.getNumberOfEventLogSubscribers(), 0U);
    EXPECT_EQ(manager.getNumberOfMetricReportSubscribers(), 0U);
}

// MetricReport subscriptions are intentionally not covered here: they cause
// EventServiceManager to construct a DbusTelemetryMonitor, which requires a
// live crow::connections::systemBus.
TEST(EventServiceManager, CountsUpdateWhenAddingSubscriptions)
{
    if constexpr (BMCWEB_REDFISH_DBUS_LOG &&
                  BMCWEB_EXPERIMENTAL_REDFISH_DBUS_LOG_SUBSCRIPTION)
    {
        GTEST_SKIP() << "EventLog subscriptions require a D-Bus connection in "
                        "this configuration";
    }

    EventServiceManager& manager = EventServiceManager::getInstance();

    persistent_data::UserSubscription eventSub;
    eventSub.destinationUrl =
        *boost::urls::parse_absolute_uri("http://127.0.0.1/event");
    eventSub.protocol = "Redfish";
    eventSub.retryPolicy = "TerminateAfterRetries";
    eventSub.eventFormatType = eventFormatType;
    eventSub.subscriptionType = subscriptionTypeSSE;

    auto eventSubscription = std::make_shared<Subscription>(
        std::make_shared<persistent_data::UserSubscription>(eventSub),
        eventSub.destinationUrl, getIoContext());

    manager.addSSESubscription(eventSubscription, "");

    EXPECT_EQ(manager.getNumberOfSubscriptions(), 1U);
    EXPECT_EQ(manager.getNumberOfEventLogSubscribers(), 1U);
    EXPECT_EQ(manager.getNumberOfMetricReportSubscribers(), 0U);

    // Clean up so this test does not affect others.
    for (const auto& id : manager.getAllIDs())
    {
        manager.deleteSubscription(id);
    }
}

} // namespace
} // namespace redfish
