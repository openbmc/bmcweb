// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "utils/hex_utils.hpp"

#include <cctype>
#include <cstdint>
#include <limits>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

using ::testing::IsEmpty;

TEST(BytesToHexString, OnSuccess)
{
    EXPECT_EQ(bytesToHexString({{0x1a, 0x2b}}), "1A2B");
}

TEST(HexCharToNibble, ReturnsCorrectNibbleForEveryHexChar)
{
    for (char c = 0; c < std::numeric_limits<char>::max(); ++c)
    {
        uint8_t expected = 16;
        if (isdigit(c) != 0)
        {
            expected = static_cast<uint8_t>(c) - '0';
        }
        else if (c >= 'A' && c <= 'F')
        {
            expected = static_cast<uint8_t>(c - 'A') + 10U;
        }
        else if (c >= 'a' && c <= 'f')
        {
            expected = static_cast<uint8_t>(c - 'a') + 10U;
        }

        EXPECT_EQ(hexCharToNibble(c), expected);
    }
}

TEST(HexStringToBytes, Success)
{
    std::vector<uint8_t> hexBytes = {0x01, 0x23, 0x45, 0x67,
                                     0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(hexStringToBytes("0123456789ABCDEF"), hexBytes);
    EXPECT_THAT(hexStringToBytes(""), IsEmpty());
}

TEST(HexStringToBytes, Failure)
{
    EXPECT_THAT(hexStringToBytes("Hello"), IsEmpty());
    EXPECT_THAT(hexStringToBytes("`"), IsEmpty());
    EXPECT_THAT(hexStringToBytes("012"), IsEmpty());
}

} // namespace
