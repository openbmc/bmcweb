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
        nlohmann::json::object_t event;

        // Default constructed should always pass
        EXPECT_TRUE(ioSub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription ioSub(url, io);
        persistent_data::UserSubscription& sub = ioSub.userSub;
        sub.resourceTypes.emplace_back("Task");
        EXPECT_FALSE(ioSub.eventMatchesFilter(event, "Event"));
        EXPECT_TRUE(ioSub.eventMatchesFilter(event, "Task"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription ioSub(url, io);
        persistent_data::UserSubscription& sub = ioSub.userSub;
        sub.registryMsgIds.emplace_back("OpenBMC.PostComplete");

        // Correct message registry
        event["MessageId"] = "OpenBMC.0.1.PostComplete";
        EXPECT_TRUE(ioSub.eventMatchesFilter(event, "Event"));

        // Different message registry
        event["MessageId"] = "Task.0.1.PostComplete";
        EXPECT_FALSE(ioSub.eventMatchesFilter(event, "Event"));

        // Different MessageId
        event["MessageId"] = "OpenBMC.0.1.NoMatch";
        EXPECT_FALSE(ioSub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription ioSub(url, io);
        event["MessageId"] = "OpenBMC.0.1.PostComplete";

        // Correct message registry
        ioSub.filter = parseFilter("MessageId eq 'OpenBMC.0.1.PostComplete'");
        EXPECT_TRUE(ioSub.eventMatchesFilter(event, "Event"));

        // Different message registry
        ioSub.filter = parseFilter("MessageId ne 'OpenBMC.0.1.PostComplete'");
        EXPECT_FALSE(ioSub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        Subscription ioSub(url, io);
        persistent_data::UserSubscription& sub = ioSub.userSub;
        event["MessageId"] = "OpenBMC.0.1.PostComplete";

        // Correct message registry
        sub.registryPrefixes.emplace_back("OpenBMC");
        EXPECT_TRUE(ioSub.eventMatchesFilter(event, "Event"));

        // Different message registry
        event["MessageId"] = "Task.0.1.PostComplete";
        EXPECT_FALSE(ioSub.eventMatchesFilter(event, "Event"));
    }
    {
        nlohmann::json::object_t event;
        // Resource types filter
        {
            Subscription ioSub(url, io);
            persistent_data::UserSubscription& sub = ioSub.userSub;
            event["OriginOfCondition"] = "/redfish/v1/Managers/bmc";

            // Correct origin
            sub.originResources.emplace_back("/redfish/v1/Managers/bmc");
            EXPECT_TRUE(ioSub.eventMatchesFilter(event, "Event"));
        }
        {
            Subscription ioSub(url, io);
            persistent_data::UserSubscription& sub = ioSub.userSub;
            // Incorrect origin
            sub.originResources.clear();
            sub.originResources.emplace_back("/redfish/v1/Managers/bmc_not");
            EXPECT_FALSE(ioSub.eventMatchesFilter(event, "Event"));
        }
    }
}
} // namespace redfish
