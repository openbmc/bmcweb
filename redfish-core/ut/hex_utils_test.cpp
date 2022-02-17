#include "utils/hex_utils.hpp"

#include <gmock/gmock.h>

TEST(ToHexString, uint64)
{
    EXPECT_EQ(intToHexString(0xFFFFFFFFFFFFFFFFULL, 16), "FFFFFFFFFFFFFFFF");

    EXPECT_EQ(intToHexString(0, 4), "0000");
    EXPECT_EQ(intToHexString(0, 8), "00000000");
    EXPECT_EQ(intToHexString(0, 12), "000000000000");
    EXPECT_EQ(intToHexString(0, 16), "0000000000000000");

    // uint64_t sized ints
    EXPECT_EQ(intToHexString(0xDEADBEEFBAD4F00DULL, 4), "F00D");
    EXPECT_EQ(intToHexString(0xDEADBEEFBAD4F00DULL, 8), "BAD4F00D");
    EXPECT_EQ(intToHexString(0xDEADBEEFBAD4F00DULL, 12), "BEEFBAD4F00D");
    EXPECT_EQ(intToHexString(0xDEADBEEFBAD4F00DULL, 16), "DEADBEEFBAD4F00D");

    // uint16_t sized ints
    EXPECT_EQ(intToHexString(0xBEEF, 1), "F");
    EXPECT_EQ(intToHexString(0xBEEF, 2), "EF");
    EXPECT_EQ(intToHexString(0xBEEF, 3), "EEF");
    EXPECT_EQ(intToHexString(0xBEEF, 4), "BEEF");
}
