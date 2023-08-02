#include "event_service_manager.hpp"
#include "event_service_store.hpp"
#include "filter_expr_printer.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <string_view>

#include <gtest/gtest.h>

namespace redfish
{

TEST(EventServiceManager, eventMatchesFilter)
{
    boost::asio::io_context io;
    boost::urls::url url;

    {
        Subscription ioSub(url, io);
        persistent_data::UserSubscription& sub = ioSub.userSub;
        nlohmann::json::object_t event;

        // Default constructed should always pass
        EXPECT_TRUE(sub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription sub(url, io);
        sub.resourceTypes.emplace_back("Task");
        EXPECT_FALSE(sub.eventMatchesFilter(event, "Event"));
        EXPECT_TRUE(sub.eventMatchesFilter(event, "Task"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription sub(url, io);
        sub.registryMsgIds.emplace_back("OpenBMC.PostComplete");

        // Correct message registry
        event["MessageId"] = "OpenBMC.0.1.PostComplete";
        EXPECT_TRUE(sub.eventMatchesFilter(event, "Event"));

        // Different message registry
        event["MessageId"] = "Task.0.1.PostComplete";
        EXPECT_FALSE(sub.eventMatchesFilter(event, "Event"));

        // Different MessageId
        event["MessageId"] = "OpenBMC.0.1.NoMatch";
        EXPECT_FALSE(sub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription sub(url, io);
        event["MessageId"] = "OpenBMC.0.1.PostComplete";

        // Correct message registry
        sub.filter = parseFilter("MessageId eq 'OpenBMC.0.1.PostComplete'");
        EXPECT_TRUE(sub.eventMatchesFilter(event, "Event"));

        // Different message registry
        sub.filter = parseFilter("MessageId ne 'OpenBMC.0.1.PostComplete'");
        EXPECT_FALSE(sub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription sub(url, io);
        event["MessageId"] = "OpenBMC.0.1.PostComplete";

        // Correct message registry
        sub.registryPrefixes.emplace_back("OpenBMC");
        EXPECT_TRUE(sub.eventMatchesFilter(event, "Event"));

        // Different message registry
        event["MessageId"] = "Task.0.1.PostComplete";
        EXPECT_FALSE(sub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        {
            Subscription sub(url, io);
            event["OriginOfCondition"] = "/redfish/v1/Managers/bmc";

            // Correct origin
            sub.originResources.emplace_back("/redfish/v1/Managers/bmc");
            EXPECT_TRUE(sub.eventMatchesFilter(event, "Event"));
        }
        {
            Subscription sub(url, io);
            // Incorrect origin
            sub.originResources.clear();
            sub.originResources.emplace_back("/redfish/v1/Managers/bmc_not");
            EXPECT_FALSE(sub.eventMatchesFilter(event, "Event"));
        }
    }
}
} // namespace redfish
