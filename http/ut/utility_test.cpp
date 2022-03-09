#include "bmcweb_config.h"

#include "utility.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace crow::utility
{
namespace
{

TEST(Utility, Base64DecodeAuthString)
{
    std::string authString("dXNlcm40bWU6cGFzc3cwcmQ=");
    std::string result;
    EXPECT_TRUE(base64Decode(authString, result));
    EXPECT_EQ(result, "usern4me:passw0rd");
}

TEST(Utility, Base64DecodeNonAscii)
{
    std::string junkString("\xff\xee\xdd\xcc\x01\x11\x22\x33");
    std::string result;
    EXPECT_FALSE(base64Decode(junkString, result));
}

TEST(Utility, Base64EncodeString)
{
    using namespace std::string_literals;
    std::string encoded;

    encoded = base64encode("");
    EXPECT_EQ(encoded, "");

    encoded = base64encode("f");
    EXPECT_EQ(encoded, "Zg==");

    encoded = base64encode("f0");
    EXPECT_EQ(encoded, "ZjA=");

    encoded = base64encode("f0\0"s);
    EXPECT_EQ(encoded, "ZjAA");

    encoded = base64encode("f0\0 "s);
    EXPECT_EQ(encoded, "ZjAAIA==");

    encoded = base64encode("f0\0 B"s);
    EXPECT_EQ(encoded, "ZjAAIEI=");

    encoded = base64encode("f0\0 Ba"s);
    EXPECT_EQ(encoded, "ZjAAIEJh");

    encoded = base64encode("f0\0 Bar"s);
    EXPECT_EQ(encoded, "ZjAAIEJhcg==");
}

TEST(Utility, Base64EncodeDecodeString)
{
    using namespace std::string_literals;
    std::string data("Data fr\0m 90 reading a \nFile"s);
    std::string encoded = base64encode(data);
    std::string decoded;
    EXPECT_TRUE(base64Decode(encoded, decoded));
    EXPECT_EQ(data, decoded);
}

TEST(Utility, GetDateTimeStdtime)
{
    using crow::utility::getDateTimeStdtime;

    // some time before the epoch
    EXPECT_EQ(getDateTimeStdtime(std::time_t{-1234567}),
              "1969-12-17T17:03:53+00:00");

    // epoch
    EXPECT_EQ(getDateTimeStdtime(std::time_t{0}), "1970-01-01T00:00:00+00:00");

    // Limits
    EXPECT_EQ(getDateTimeStdtime(std::numeric_limits<std::time_t>::max()),
              "9999-12-31T23:59:59+00:00");
    EXPECT_EQ(getDateTimeStdtime(std::numeric_limits<std::time_t>::min()),
              "1970-01-01T00:00:00+00:00");
}

TEST(Utility, GetDateTimeUint)
{
    EXPECT_EQ(getDateTimeUint(uint64_t{1638312095}),
              "2021-11-30T22:41:35+00:00");
    // some time in the future, beyond 2038
    EXPECT_EQ(getDateTimeUint(uint64_t{41638312095}),
              "3289-06-18T21:48:15+00:00");
    // the maximum time we support
    EXPECT_EQ(getDateTimeUint(uint64_t{253402300799}),
              "9999-12-31T23:59:59+00:00");

    // returns the maximum Redfish date
    EXPECT_EQ(getDateTimeUint(std::numeric_limits<uint64_t>::max()),
              "9999-12-31T23:59:59+00:00");

    EXPECT_EQ(getDateTimeUint(std::numeric_limits<uint64_t>::min()),
              "1970-01-01T00:00:00+00:00");
}

TEST(Utility, GetDateTimeUintMs)
{
    // returns the maximum Redfish date
    EXPECT_EQ(getDateTimeUintMs(std::numeric_limits<uint64_t>::max()),
              "9999-12-31T23:59:59.999000+00:00");
    EXPECT_EQ(getDateTimeUintMs(std::numeric_limits<uint64_t>::min()),
              "1970-01-01T00:00:00+00:00");
}

TEST(Utility, UrlFromPieces)
{
    using crow::utility::urlFromPieces;
    boost::urls::url url = urlFromPieces("redfish", "v1", "foo");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/redfish/v1/foo");

    url = urlFromPieces("/", "badString");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/%2f/badString");

    url = urlFromPieces("bad?tring");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/bad%3ftring");

    url = urlFromPieces("/", "bad&tring");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/%2f/bad&tring");
}

TEST(Utility, ValidateAndSplitUrlPositive)
{
    using crow::utility::validateAndSplitUrl;
    std::string host;
    std::string urlProto;
    std::string port;
    std::string path;
    ASSERT_TRUE(validateAndSplitUrl("https://foo.com:18080/bar", urlProto, host,
                                    port, path));
    EXPECT_EQ(host, "foo.com");
    EXPECT_EQ(urlProto, "https");
    EXPECT_EQ(port, "18080");

    EXPECT_EQ(path, "/bar");

    // query string
    ASSERT_TRUE(validateAndSplitUrl("https://foo.com:18080/bar?foobar=1",
                                    urlProto, host, port, path));
    EXPECT_EQ(path, "/bar?foobar=1");

    // Missing port
    ASSERT_TRUE(
        validateAndSplitUrl("https://foo.com/bar", urlProto, host, port, path));
    EXPECT_EQ(port, "443");

    // If http push eventing is allowed, allow http, if it's not, parse should
    // fail.
#ifdef BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING
    ASSERT_TRUE(
        validateAndSplitUrl("http://foo.com/bar", urlProto, host, port, path));
    EXPECT_EQ(port, "80");
#else
    ASSERT_FALSE(
        validateAndSplitUrl("http://foo.com/bar", urlProto, host, port, path));
#endif
}

TEST(Router, ParameterTagging)
{
    EXPECT_EQ(6 * 6 + 6 * 3 + 2,
              crow::black_magic::getParameterTag("<uint><double><int>"));
    EXPECT_EQ(1, crow::black_magic::getParameterTag("<int>"));
    EXPECT_EQ(2, crow::black_magic::getParameterTag("<uint>"));
    EXPECT_EQ(3, crow::black_magic::getParameterTag("<float>"));
    EXPECT_EQ(3, crow::black_magic::getParameterTag("<double>"));
    EXPECT_EQ(4, crow::black_magic::getParameterTag("<str>"));
    EXPECT_EQ(4, crow::black_magic::getParameterTag("<string>"));
    EXPECT_EQ(5, crow::black_magic::getParameterTag("<path>"));
    EXPECT_EQ(6 * 6 + 6 + 1,
              crow::black_magic::getParameterTag("<int><int><int>"));
    EXPECT_EQ(6 * 6 + 6 + 2,
              crow::black_magic::getParameterTag("<uint><int><int>"));
    EXPECT_EQ(6 * 6 + 6 * 3 + 2,
              crow::black_magic::getParameterTag("<uint><double><int>"));
}

} // namespace
} // namespace crow::utility
