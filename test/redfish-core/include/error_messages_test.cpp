// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "error_messages.hpp"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace redfish::messages
{
namespace
{

TEST(ErrorMessages, StringArgumentIsRenderedWithoutQuotes)
{
    nlohmann::json msg = propertyValueNotInList("BogusValue", "HTTPBasicAuth");
    EXPECT_EQ(msg["MessageArgs"],
              nlohmann::json::array({"BogusValue", "HTTPBasicAuth"}));
}

TEST(ErrorMessages, NonStringArgumentKeepsJsonRendering)
{
    nlohmann::json boolMsg = propertyValueNotInList(true, "Locked");
    EXPECT_EQ(boolMsg["MessageArgs"],
              nlohmann::json::array({"true", "Locked"}));

    nlohmann::json intMsg =
        propertyValueOutOfRange(-5, "AccountLockoutDuration");
    EXPECT_EQ(intMsg["MessageArgs"],
              nlohmann::json::array({"-5", "AccountLockoutDuration"}));
}

} // namespace
} // namespace redfish::messages
