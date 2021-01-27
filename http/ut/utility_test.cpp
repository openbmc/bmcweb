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
    std::string data("Test\0 Data2 \0Set 30"s);
    std::string encoded = crow::utility::base64encode(data);
    std::string decoded;
    EXPECT_TRUE(crow::utility::base64Decode(encoded, decoded));
    EXPECT_EQ(data, decoded);
}

TEST(Utility, Base64EncodeStream)
{
    using namespace std::string_literals;
    std::istringstream data("Data fr\0m 90 reading a \nFile"s);
    std::vector<char> charData((std::istreambuf_iterator<char>(data)),
                               std::istreambuf_iterator<char>());
    std::string stringData(charData.data(), charData.size());
    std::string encoded = crow::utility::base64encode(stringData);
    std::string decoded;
    EXPECT_TRUE(crow::utility::base64Decode(encoded, decoded));
    EXPECT_EQ(stringData, decoded);
}
