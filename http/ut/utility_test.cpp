#include "utility.hpp"

#include "gmock/gmock.h"

TEST(Utility, Base64DecodeAuthString)
{
    std::string authString("dXNlcm40bWU6cGFzc3cwcmQ=");
    std::string result;
    EXPECT_TRUE(crow::utility::base64Decode(authString, result));
    EXPECT_EQ(result, "usern4me:passw0rd");
}

TEST(Utility, Base64DecodeNonAscii)
{
    std::string junkString("\xff\xee\xdd\xcc\x01\x11\x22\x33");
    std::string result;
    EXPECT_FALSE(crow::utility::base64Decode(junkString, result));
}

TEST(Utility, Base64EncodeString)
{
    using namespace std::string_literals;
    std::string encoded;

    encoded = crow::utility::base64encode("");
    EXPECT_EQ(encoded, "");

    encoded = crow::utility::base64encode("f");
    EXPECT_EQ(encoded, "Zg==");

    encoded = crow::utility::base64encode("f0");
    EXPECT_EQ(encoded, "ZjA=");

    encoded = crow::utility::base64encode("f0\0"s);
    EXPECT_EQ(encoded, "ZjAA");

    encoded = crow::utility::base64encode("f0\0 "s);
    EXPECT_EQ(encoded, "ZjAAIA==");

    encoded = crow::utility::base64encode("f0\0 B"s);
    EXPECT_EQ(encoded, "ZjAAIEI=");

    encoded = crow::utility::base64encode("f0\0 Ba"s);
    EXPECT_EQ(encoded, "ZjAAIEJh");

    encoded = crow::utility::base64encode("f0\0 Bar"s);
    EXPECT_EQ(encoded, "ZjAAIEJhcg==");
}

TEST(Utility, Base64EncodeDecodeString)
{
    using namespace std::string_literals;
    std::string data("Data fr\0m 90 reading a \nFile"s);
    std::string encoded = crow::utility::base64encode(data);
    std::string decoded;
    EXPECT_TRUE(crow::utility::base64Decode(encoded, decoded));
    EXPECT_EQ(data, decoded);
}
