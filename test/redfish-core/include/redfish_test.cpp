#include "bmcweb_config.h"

#include "app.hpp"
#include "event_service_manager.hpp"
#include "event_service_store.hpp"
#include "io_context_singleton.hpp"
#include "redfish.hpp"
#include "subscription.hpp"

#include <boost/scope/scope_exit.hpp>
#include <boost/url/parse.hpp>

#include <cstddef>
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

class EventServiceManagerTest : public testing::Test
{
  public:
    EventServiceManagerTest() = default;
    EventServiceManagerTest(const EventServiceManagerTest&) = delete;
    EventServiceManagerTest& operator=(const EventServiceManagerTest&) = delete;
    EventServiceManagerTest(EventServiceManagerTest&&) = delete;
    EventServiceManagerTest& operator=(EventServiceManagerTest&&) = delete;

    ~EventServiceManagerTest() override
    {
        for (const std::string& id : manager.getAllIDs())
        {
            manager.deleteSubscription(id);
        }
    }

  protected:
    EventServiceManager manager;

    void SetUp() override
    {
        if constexpr (BMCWEB_REDFISH_DBUS_LOG &&
                      BMCWEB_EXPERIMENTAL_REDFISH_DBUS_LOG_SUBSCRIPTION)
        {
            GTEST_SKIP() << "EventLog subscriptions require a D-Bus connection "
                            "in this configuration";
        }
    }

    static std::shared_ptr<Subscription> makeSubscription()
    {
        auto userSub = std::make_shared<persistent_data::UserSubscription>();
        userSub->destinationUrl =
            *boost::urls::parse_absolute_uri("http://127.0.0.1/event");
        userSub->protocol = "Redfish";
        userSub->retryPolicy = "TerminateAfterRetries";
        userSub->eventFormatType = eventFormatType;
        userSub->subscriptionType = "RedfishEvent";
        return std::make_shared<Subscription>(userSub, userSub->destinationUrl,
                                              getIoContext());
    }
};

TEST_F(EventServiceManagerTest, InitiallyHasNoSubscriptions)
{
    EXPECT_EQ(manager.getNumberOfSubscriptions(), 0U);
    EXPECT_EQ(manager.getNumberOfSSESubscriptions(), 0U);
    EXPECT_TRUE(manager.getAllIDs().empty());
}

TEST_F(EventServiceManagerTest, RegistersAndRemovesPushSubscription)
{
    auto& subscriptions = persistent_data::EventServiceStore::getInstance()
                              .subscriptionsConfigMap;
    const size_t initialStoreSize = subscriptions.size();
    std::shared_ptr<Subscription> eventSubscription = makeSubscription();

    std::string id = manager.addPushSubscription(eventSubscription);
    ASSERT_FALSE(id.empty());

    EXPECT_EQ(manager.getNumberOfSubscriptions(), 1U);
    EXPECT_EQ(manager.getNumberOfSSESubscriptions(), 0U);
    EXPECT_TRUE(manager.isSubscriptionExist(id));
    EXPECT_EQ(manager.getSubscription(id), eventSubscription);
    EXPECT_EQ(eventSubscription->userSub->id, id);
    EXPECT_THAT(manager.getAllIDs(), testing::ElementsAre(id));
    ASSERT_TRUE(subscriptions.contains(id));
    EXPECT_EQ(subscriptions.at(id), eventSubscription->userSub);
    EXPECT_EQ(subscriptions.size(), initialStoreSize + 1);

    EXPECT_TRUE(manager.deleteSubscription(id));

    EXPECT_FALSE(manager.isSubscriptionExist(id));
    EXPECT_EQ(manager.getSubscription(id), nullptr);
    EXPECT_EQ(manager.getNumberOfSubscriptions(), 0U);
    EXPECT_EQ(manager.getNumberOfSSESubscriptions(), 0U);
    EXPECT_TRUE(manager.getAllIDs().empty());
    EXPECT_FALSE(subscriptions.contains(id));
    EXPECT_EQ(subscriptions.size(), initialStoreSize);
    EXPECT_FALSE(manager.deleteSubscription(id));
}

TEST_F(EventServiceManagerTest, UnknownIdDoesNotChangeSubscriptions)
{
    auto& subscriptions = persistent_data::EventServiceStore::getInstance()
                              .subscriptionsConfigMap;
    const size_t initialStoreSize = subscriptions.size();

    EXPECT_FALSE(manager.isSubscriptionExist("missing"));
    EXPECT_EQ(manager.getSubscription("missing"), nullptr);
    EXPECT_FALSE(manager.deleteSubscription("missing"));
    EXPECT_FALSE(manager.deleteSubscription(""));
    EXPECT_EQ(manager.getNumberOfSubscriptions(), 0U);
    EXPECT_EQ(manager.getNumberOfSSESubscriptions(), 0U);
    EXPECT_TRUE(manager.getAllIDs().empty());
    EXPECT_EQ(subscriptions.size(), initialStoreSize);
}

TEST_F(EventServiceManagerTest, RemovesOnlyTheRequestedSubscription)
{
    std::shared_ptr<Subscription> first = makeSubscription();
    std::shared_ptr<Subscription> second = makeSubscription();
    std::shared_ptr<Subscription> third = makeSubscription();
    std::string firstId = manager.addPushSubscription(first);
    ASSERT_FALSE(firstId.empty());
    std::string secondId = manager.addPushSubscription(second);
    ASSERT_FALSE(secondId.empty());
    std::string thirdId = manager.addPushSubscription(third);
    ASSERT_FALSE(thirdId.empty());

    EXPECT_NE(firstId, secondId);
    EXPECT_NE(firstId, thirdId);
    EXPECT_NE(secondId, thirdId);
    EXPECT_EQ(manager.getNumberOfSubscriptions(), 3U);
    EXPECT_EQ(manager.getNumberOfSSESubscriptions(), 0U);
    EXPECT_THAT(manager.getAllIDs(),
                testing::UnorderedElementsAre(firstId, secondId, thirdId));

    ASSERT_TRUE(manager.deleteSubscription(secondId));
    EXPECT_FALSE(manager.deleteSubscription(secondId));
    EXPECT_EQ(manager.getNumberOfSubscriptions(), 2U);
    EXPECT_EQ(manager.getSubscription(firstId), first);
    EXPECT_EQ(manager.getSubscription(thirdId), third);
    EXPECT_THAT(manager.getAllIDs(),
                testing::UnorderedElementsAre(firstId, thirdId));

    const auto& subscriptions =
        persistent_data::EventServiceStore::getInstance()
            .subscriptionsConfigMap;
    EXPECT_TRUE(subscriptions.contains(firstId));
    EXPECT_FALSE(subscriptions.contains(secondId));
    EXPECT_TRUE(subscriptions.contains(thirdId));
}

TEST_F(EventServiceManagerTest, ScopedCleanupRestoresSubscriptionState)
{
    const auto& subscriptions =
        persistent_data::EventServiceStore::getInstance()
            .subscriptionsConfigMap;
    const size_t initialStoreSize = subscriptions.size();
    std::string id;
    {
        boost::scope::scope_exit cleanup([this] {
            for (const std::string& subscriptionId : manager.getAllIDs())
            {
                manager.deleteSubscription(subscriptionId);
            }
        });
        id = manager.addPushSubscription(makeSubscription());
        ASSERT_FALSE(id.empty());
        ASSERT_TRUE(manager.isSubscriptionExist(id));
        ASSERT_TRUE(subscriptions.contains(id));
    }

    EXPECT_FALSE(manager.isSubscriptionExist(id));
    EXPECT_FALSE(subscriptions.contains(id));
    EXPECT_EQ(subscriptions.size(), initialStoreSize);
    EXPECT_EQ(manager.getNumberOfSubscriptions(), 0U);
    EXPECT_EQ(manager.getNumberOfSSESubscriptions(), 0U);
}

TEST_F(EventServiceManagerTest, UpdatesPersistedServiceConfiguration)
{
    auto& store = persistent_data::EventServiceStore::getInstance();
    persistent_data::EventServiceConfig original =
        store.getEventServiceConfig();
    boost::scope::scope_exit restore([this, &original] {
        manager.setEventServiceConfig(original);
    });

    persistent_data::EventServiceConfig config = original;
    config.enabled = !original.enabled;
    config.retryAttempts = original.retryAttempts + 1;
    config.retryTimeoutInterval = original.retryTimeoutInterval + 1;
    manager.setEventServiceConfig(config);

    persistent_data::EventServiceConfig actual = store.getEventServiceConfig();
    EXPECT_EQ(actual.enabled, config.enabled);
    EXPECT_EQ(actual.retryAttempts, config.retryAttempts);
    EXPECT_EQ(actual.retryTimeoutInterval, config.retryTimeoutInterval);
}

} // namespace
} // namespace redfish
