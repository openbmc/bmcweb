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

TEST(BytesToHexString, Success)
{
    EXPECT_EQ(bytesToHexString({0x1a, 0x2b}), "1A2B");
}

TEST(HexCharToNibble, chars)
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
            expected = static_cast<uint8_t>(c) - 'A' + 10;
        }
        else if (c >= 'a' && c <= 'f')
        {
            expected = static_cast<uint8_t>(c) - 'a' + 10;
        }

        EXPECT_EQ(hexCharToNibble(c), expected);
    }
}

TEST(HexStringToBytes, Success)
{
    std::vector<uint8_t> hexBytes = {0x01, 0x23, 0x45, 0x67,
                                     0x89, 0xAB, 0xCD, 0xEF};
    EXPECT_EQ(hexStringToBytes("0123456789ABCDEF"), hexBytes);
    EXPECT_TRUE(hexStringToBytes("").empty());
}

TEST(HexStringToBytes, Failure)
{
    EXPECT_TRUE(hexStringToBytes("Hello").empty());
    EXPECT_TRUE(hexStringToBytes("`").empty());
    EXPECT_TRUE(hexStringToBytes("012").empty());
}
