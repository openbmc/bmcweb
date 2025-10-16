// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "registries.hpp"
#include "registries/openbmc_message_registry.hpp"

#include <optional>

#include <gtest/gtest.h>

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
            "Non-Existent", redfish::registries::Openbmc::registry);
    ASSERT_EQ(msg, nullptr);

    const redfish::registries::Message* msg1 =
        redfish::registries::getMessageFromRegistry(
            "ServiceStarted", redfish::registries::Openbmc::registry);
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

TEST(RedfishRegistries, GetMessageComponents)
{
    std::optional<registries::MessageId> msgComponents =
        registries::getMessageComponents("OpenBMC.5.threeComponents");
    ASSERT_EQ(msgComponents, std::nullopt);

    msgComponents =
        registries::getMessageComponents("OpenBMC.0.0.5.fiveComponents");
    ASSERT_EQ(msgComponents, std::nullopt);

    msgComponents =
        registries::getMessageComponents("OpenBMC.0.5.BIOSAttributesChanged");
    ASSERT_NE(msgComponents, std::nullopt);
    EXPECT_EQ(msgComponents->registryName, "OpenBMC");
    EXPECT_EQ(msgComponents->messageKey, "BIOSAttributesChanged");
    EXPECT_EQ(msgComponents->majorVersion, "0");
    EXPECT_EQ(msgComponents->minorVersion, "5");
}

} // namespace
} // namespace redfish::registries
