#include "registries.hpp"
#include "registries/openbmc_message_registry.hpp"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <cstring>

#include "gmock/gmock.h"

TEST(RedfishRegistries, fillMessageArgs)
{
    using redfish::registries::fillMessageArgs;
    std::string toFill("%1");
    fillMessageArgs({{"foo"}}, toFill);
    EXPECT_EQ(toFill, "foo");

    toFill = "";
    fillMessageArgs({}, toFill);
    EXPECT_EQ(toFill, "");

    toFill = "%1, %2";
    fillMessageArgs({{"foo", "bar"}}, toFill);
    EXPECT_EQ(toFill, "foo, bar");
}

TEST(RedfishRegistries, serviceStartedMessage)
{
    const redfish::registries::Message* msg =
        redfish::registries::getMessageFromRegistry(
            "Non-Existent", redfish::registries::openbmc::registry);
    ASSERT_EQ(msg, nullptr);

    const redfish::registries::Message* msg1 =
        redfish::registries::getMessageFromRegistry(
            "ServiceStarted", redfish::registries::openbmc::registry);
    ASSERT_NE(msg1, nullptr);

    const redfish::registries::Message* msg2 =
        redfish::registries::getMessage("OpenBMC.1.0.Non_Existent_Message");
    ASSERT_EQ(msg2, nullptr);
}
