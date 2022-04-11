#include "registries.hpp"
#include "registries/openbmc_message_registry.hpp"

#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace redfish::registries
{
namespace
{

TEST(FillMessageArgs, ArgsAreFilledCorrectly)
{
    EXPECT_EQ(fillMessageArgs({{"foo"}}, "%1"), "foo");
    EXPECT_EQ(fillMessageArgs({}, ""), "");
    EXPECT_EQ(fillMessageArgs({{"foo", "bar"}}, "%1, %2"), "foo, bar");
    EXPECT_EQ(fillMessageArgs({{"foo"}}, "%1 bar"), "foo bar");
    EXPECT_EQ(fillMessageArgs({}, "%1"), "");
    EXPECT_EQ(fillMessageArgs({}, "%"), "");
    EXPECT_EQ(fillMessageArgs({}, "%foo"), "");
}

TEST(RedfishRegistries, GetMessageFromRegistry)
{
    const redfish::registries::Message* msg =
        redfish::registries::getMessageFromRegistry(
            "Non-Existent", redfish::registries::openbmc::registry);
    ASSERT_EQ(msg, nullptr);

    const redfish::registries::Message* msg1 =
        redfish::registries::getMessageFromRegistry(
            "ServiceStarted", redfish::registries::openbmc::registry);
    ASSERT_NE(msg1, nullptr);

    EXPECT_EQ(std::string(msg1->description),
              "Indicates that a service has started successfully.");
    EXPECT_EQ(std::string(msg1->message),
              "Service %1 has started successfully.");
    EXPECT_EQ(std::string(msg1->messageSeverity), "OK");
    EXPECT_EQ(msg1->numberOfArgs, 1);
    EXPECT_EQ(std::string(msg1->resolution), "None.");
}

TEST(RedfishRegistries, GetMessage)
{
    const redfish::registries::Message* msg =
        redfish::registries::getMessage("OpenBMC.1.0.Non_Existent_Message");
    ASSERT_EQ(msg, nullptr);

    msg = redfish::registries::getMessage("OpenBMC.1.0.ServiceStarted");
    ASSERT_NE(msg, nullptr);
}

} // namespace
} // namespace redfish::registries
