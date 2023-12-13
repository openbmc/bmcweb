
#include "utility.hpp"

#include <filesystem>
#include <fstream>
#include <thread>

#include "gtest/gtest.h"
namespace
{

inline std::string generateBigdata()
{
    std::string result;
    size_t i = 0;
    while (i < 10000)
    {
        result += "sample text";
        i += std::string("sample text").length();
    }
    return result;
}
std::string testString(std::string_view input)
{
    std::string encoded;
    crow::utility::Base64Encoder encoder;
    encoder.encode(input, encoded);
    encoder.finalize(encoded);
    std::string decoded;
    crow::utility::base64Decode(encoded, decoded);
    EXPECT_EQ(decoded, input);
    return encoded;
}
TEST(Base64Encoder, SmallString)
{
    EXPECT_EQ(testString("sample text"), "c2FtcGxlIHRleHQ=");
    EXPECT_EQ(testString("a"), "YQ==");
    EXPECT_EQ(testString("ab"), "YWI=");
    EXPECT_EQ(testString("abc"), "YWJj");
    EXPECT_EQ(testString("abcd"), "YWJjZA==");
    EXPECT_EQ(testString("124"), "MTI0");
    EXPECT_EQ(testString("1245"), "MTI0NQ==");
    EXPECT_EQ(testString("&-?"), "Ji0/");
    EXPECT_EQ(testString("&-?="), "Ji0/PQ==");
    EXPECT_EQ(testString("&-?=="), "Ji0/PT0=");
}
TEST(Base64Encoder, LargeString)
{
    std::string bigdata = generateBigdata();
    testString(bigdata);
    bigdata += "a";
    testString(bigdata);
    bigdata += "b";
    testString(bigdata);
    bigdata += "c";
    testString(bigdata);
}
} // namespace
