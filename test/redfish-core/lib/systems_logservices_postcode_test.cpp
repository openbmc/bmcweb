// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "systems_logservices_postcodes.hpp"

#include <cstdint>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(LogServicesPostCodeParse, PostCodeParse)
{
    uint64_t currentValue = 0;
    uint16_t index = 0;
    EXPECT_TRUE(parsePostCode("B1-2", currentValue, index));
    EXPECT_EQ(currentValue, 2);
    EXPECT_EQ(index, 1);
    EXPECT_TRUE(parsePostCode("B200-300", currentValue, index));
    EXPECT_EQ(currentValue, 300);
    EXPECT_EQ(index, 200);

    EXPECT_FALSE(parsePostCode("", currentValue, index));
    EXPECT_FALSE(parsePostCode("B", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1-", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2z", currentValue, index));
    // Uint16_t max + 1
    EXPECT_FALSE(parsePostCode("B65536-1", currentValue, index));

    // Uint64_t max + 1
    EXPECT_FALSE(parsePostCode("B1-18446744073709551616", currentValue, index));

    // Negative numbers
    EXPECT_FALSE(parsePostCode("B-1-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B-1--2", currentValue, index));
}

} // namespace
} // namespace redfish
