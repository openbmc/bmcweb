#include "event_matches_filter.hpp"
#include "event_service_store.hpp"

#include <nlohmann/json.hpp>

#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{

TEST(EventServiceManager, eventMatchesFilter)
{
    {
        persistent_data::UserSubscription sub;
        nlohmann::json::object_t event;

        // Default constructed should always pass
        EXPECT_TRUE(eventMatchesFilter(sub, event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        persistent_data::UserSubscription sub;
        sub.resourceTypes.emplace_back("Task");
        EXPECT_FALSE(eventMatchesFilter(sub, event, "Event"));
        EXPECT_TRUE(eventMatchesFilter(sub, event, "Task"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        persistent_data::UserSubscription sub;
        sub.registryMsgIds.emplace_back("OpenBMC.PostComplete");

        // Correct message registry
        event["MessageId"] = "OpenBMC.0.1.PostComplete";
        EXPECT_TRUE(eventMatchesFilter(sub, event, "Event"));

        // Different message registry
        event["MessageId"] = "Task.0.1.PostComplete";
        EXPECT_FALSE(eventMatchesFilter(sub, event, "Event"));

        // Different MessageId
        event["MessageId"] = "OpenBMC.0.1.NoMatch";
        EXPECT_FALSE(eventMatchesFilter(sub, event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        persistent_data::UserSubscription sub;
        event["MessageId"] = "OpenBMC.0.1.PostComplete";

        // Correct message registry
        sub.registryPrefixes.emplace_back("OpenBMC");
        EXPECT_TRUE(eventMatchesFilter(sub, event, "Event"));

        // Different message registry
        event["MessageId"] = "Task.0.1.PostComplete";
        EXPECT_FALSE(eventMatchesFilter(sub, event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        {
            persistent_data::UserSubscription sub;
            event["OriginOfCondition"] = "/redfish/v1/Managers/bmc";

            // Correct origin
            sub.originResources.emplace_back("/redfish/v1/Managers/bmc");
            EXPECT_TRUE(eventMatchesFilter(sub, event, "Event"));
        }
        {
            persistent_data::UserSubscription sub;
            // Incorrect origin
            sub.originResources.clear();
            sub.originResources.emplace_back("/redfish/v1/Managers/bmc_not");
            EXPECT_FALSE(eventMatchesFilter(sub, event, "Event"));
        }
    }
}
} // namespace redfish
