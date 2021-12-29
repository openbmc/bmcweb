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

TEST(Utility, GetDateTime)
{
    // some time before the epoch
    EXPECT_EQ(crow::utility::getDateTimeStdtime(std::time_t{-1234567}),
              "1969-12-17T17:03:53+00:00");
    // epoch
    EXPECT_EQ(crow::utility::getDateTimeStdtime(std::time_t{0}),
              "1970-01-01T00:00:00+00:00");
    // some time in the past after the epoch
    EXPECT_EQ(crow::utility::getDateTimeUint(uint64_t{1638312095}),
              "2021-11-30T22:41:35+00:00");
    // some time in the future, beyond 2038
    EXPECT_EQ(crow::utility::getDateTimeUint(uint64_t{41638312095}),
              "3289-06-18T21:48:15+00:00");
    // the maximum time we support
    EXPECT_EQ(crow::utility::getDateTimeUint(uint64_t{253402300799}),
              "9999-12-31T23:59:59+00:00");
}

TEST(Utility, GetDateTimeChronoApi)
{
    // epoch
    EXPECT_EQ(crow::utility::getDateTime(std::chrono::seconds{0}),
              "1970-01-01T00:00:00+00:00");
    // some time in the past after the epoch
    EXPECT_EQ(crow::utility::getDateTime(std::chrono::seconds{1638312095}),
              "2021-11-30T22:41:35+00:00");
    EXPECT_EQ(
        crow::utility::getDateTime(std::chrono::milliseconds{1638312095000}),
        "2021-11-30T22:41:35+00:00");
    // some time in the future, beyond 2038
    EXPECT_EQ(crow::utility::getDateTime(std::chrono::years{400}),
              "2370-01-01T00:00:00+00:00");
}
