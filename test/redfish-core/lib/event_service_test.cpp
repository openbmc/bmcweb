#include "event_matches_filter.hpp"
#include "event_service.hpp"
#include "event_service_store.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(NormalizeSubscriptionMessageId, AcceptsRegistryAndMessageKey)
{
    EXPECT_EQ(normalizeSubscriptionMessageId("Base.Success"), "Base.Success");
    EXPECT_EQ(normalizeSubscriptionMessageId("TaskEvent.TaskStarted"),
              "TaskEvent.TaskStarted");
}

TEST(NormalizeSubscriptionMessageId, IgnoresVersion)
{
    EXPECT_EQ(normalizeSubscriptionMessageId("Base.1.0.Success"),
              "Base.Success");
    EXPECT_EQ(normalizeSubscriptionMessageId("Base.99.99.Success"),
              "Base.Success");
}

TEST(NormalizeSubscriptionMessageId, RejectsUnknownRegistryOrMessageKey)
{
    EXPECT_EQ(normalizeSubscriptionMessageId("Unknown.Success"), std::nullopt);
    EXPECT_EQ(normalizeSubscriptionMessageId("Base.UnknownMessage"),
              std::nullopt);
    EXPECT_EQ(normalizeSubscriptionMessageId("Base.TaskStarted"), std::nullopt);
}

TEST(NormalizeSubscriptionMessageId, RejectsMalformedIds)
{
    for (const char* messageId :
         {"", "Success", ".Success", "Base.", "Base.1.Success",
          "Base..0.Success", "Base.1..Success", "Base.x.0.Success",
          "Base.1.0.Success.Extra"})
    {
        EXPECT_EQ(normalizeSubscriptionMessageId(messageId), std::nullopt)
            << messageId;
    }
}

TEST(NormalizeSubscriptionMessageId, NormalizedIdsMatchEvents)
{
    for (const char* messageId : {"Base.Success", "Base.1.0.Success"})
    {
        std::optional<std::string> normalizedId =
            normalizeSubscriptionMessageId(messageId);
        ASSERT_TRUE(normalizedId.has_value());

        persistent_data::UserSubscription subscription;
        subscription.registryMsgIds.push_back(*normalizedId);
        nlohmann::json::object_t event;
        event["MessageId"] = "Base.1.0.Success";
        EXPECT_TRUE(eventMatchesFilter(subscription, event, "Event"));

        event["MessageId"] = "Base.1.0.InternalError";
        EXPECT_FALSE(eventMatchesFilter(subscription, event, "Event"));
        event["MessageId"] = "TaskEvent.1.0.Success";
        EXPECT_FALSE(eventMatchesFilter(subscription, event, "Event"));
    }
}

} // namespace
} // namespace redfish
