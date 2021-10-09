#include "utils/hex_utils.hpp"

#include <gmock/gmock.h>

TEST(ToHexString, uint64)
{
    EXPECT_EQ(intToHexString(0xFFFFFFFFFFFFFFFFULL), "FFFFFFFFFFFFFFFF");

    EXPECT_EQ(intToHexString<uint64_t>(0), "0000000000000000");
    EXPECT_EQ(intToHexString<uint64_t>(0, 4), "0000");
    EXPECT_EQ(intToHexString<uint64_t>(0, 8), "00000000");
    EXPECT_EQ(intToHexString<uint64_t>(0, 12), "000000000000");
    EXPECT_EQ(intToHexString<uint64_t>(0, 16), "0000000000000000");


    uint64_t deadbeef = 0xDEADBEEFBAD4F00DULL;
    EXPECT_EQ(intToHexString<uint64_t>(deadbeef), "DEADBEEFBAD4F00D");
    EXPECT_EQ(intToHexString<uint64_t>(deadbeef, 4), "F00D");
    EXPECT_EQ(intToHexString<uint64_t>(deadbeef, 8), "BAD4F00D");
    EXPECT_EQ(intToHexString<uint64_t>(deadbeef, 12), "BEEFBAD4F00D");
    EXPECT_EQ(intToHexString<uint64_t>(deadbeef, 16), "DEADBEEFBAD4F00D");
}

TEST(ToHexString, uint16)
{
    uint16_t beef = 0xBEEF;
    EXPECT_EQ(intToHexString(beef), "BEEF");
    EXPECT_EQ(intToHexString(beef, 1), "F");
    EXPECT_EQ(intToHexString(beef, 2), "EF");
    EXPECT_EQ(intToHexString(beef, 3), "EEF");
    EXPECT_EQ(intToHexString(beef, 4), "BEEF");
}