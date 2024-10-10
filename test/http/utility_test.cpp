
#include "utility.hpp"

#include <boost/system/result.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>

#include <algorithm>
#include <ctime>
#include <functional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace bmcweb::utility
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

TEST(Utility, Base64Encoder)
{
    using namespace std::string_literals;
    std::string data = "f0\0 Bar"s;
    for (size_t chunkSize = 1; chunkSize < 6; chunkSize++)
    {
        std::string_view testString(data);
        std::string out;
        Base64Encoder encoder;
        while (!testString.empty())
        {
            size_t thisChunk = std::min(testString.size(), chunkSize);
            encoder.encode(testString.substr(0, thisChunk), out);
            testString.remove_prefix(thisChunk);
        }

        encoder.finalize(out);
        EXPECT_EQ(out, "ZjAAIEJhcg==");
    }
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

TEST(Utility, readUrlSegments)
{
    boost::system::result<boost::urls::url_view> parsed =
        boost::urls::parse_relative_ref("/redfish/v1/Chassis#/Fans/0/Reading");

    EXPECT_TRUE(readUrlSegments(*parsed, "redfish", "v1", "Chassis"));

    EXPECT_FALSE(readUrlSegments(*parsed, "FOOBAR", "v1", "Chassis"));

    EXPECT_FALSE(readUrlSegments(*parsed, "redfish", "v1"));

    EXPECT_FALSE(
        readUrlSegments(*parsed, "redfish", "v1", "Chassis", "FOOBAR"));

    std::string out1;
    std::string out2;
    std::string out3;
    EXPECT_TRUE(readUrlSegments(*parsed, "redfish", "v1", std::ref(out1)));
    EXPECT_EQ(out1, "Chassis");

    out1 = out2 = out3 = "";
    EXPECT_TRUE(readUrlSegments(*parsed, std::ref(out1), std::ref(out2),
                                std::ref(out3)));
    EXPECT_EQ(out1, "redfish");
    EXPECT_EQ(out2, "v1");
    EXPECT_EQ(out3, "Chassis");

    out1 = out2 = out3 = "";
    EXPECT_TRUE(readUrlSegments(*parsed, "redfish", std::ref(out1), "Chassis"));
    EXPECT_EQ(out1, "v1");

    out1 = out2 = out3 = "";
    EXPECT_TRUE(readUrlSegments(*parsed, std::ref(out1), "v1", std::ref(out2)));
    EXPECT_EQ(out1, "redfish");
    EXPECT_EQ(out2, "Chassis");

    EXPECT_FALSE(readUrlSegments(*parsed, "too", "short"));

    EXPECT_FALSE(readUrlSegments(*parsed, "too", "long", "too", "long"));

    EXPECT_FALSE(
        readUrlSegments(*parsed, std::ref(out1), "v2", std::ref(out2)));

    EXPECT_FALSE(readUrlSegments(*parsed, "redfish", std::ref(out1),
                                 std::ref(out2), std::ref(out3)));

    parsed = boost::urls::parse_relative_ref("/absolute/url");
    EXPECT_TRUE(readUrlSegments(*parsed, "absolute", "url"));

    parsed = boost::urls::parse_relative_ref("not/absolute/url");
    EXPECT_FALSE(readUrlSegments(*parsed, "not", "absolute", "url"));

    parsed = boost::urls::parse_relative_ref("/excellent/path");

    EXPECT_TRUE(readUrlSegments(*parsed, "excellent", "path", OrMorePaths()));
    EXPECT_TRUE(readUrlSegments(*parsed, "excellent", OrMorePaths()));
    EXPECT_TRUE(readUrlSegments(*parsed, OrMorePaths()));
}

TEST(Router, ParameterTagging)
{
    EXPECT_EQ(1, getParameterTag("<str>"));
    EXPECT_EQ(1, getParameterTag("<string>"));
    EXPECT_EQ(1, getParameterTag("<path>"));
    EXPECT_EQ(2, getParameterTag("<str>/<str>"));
    EXPECT_EQ(2, getParameterTag("<string>/<string>"));
    EXPECT_EQ(2, getParameterTag("<path>/<path>"));
}

TEST(URL, JsonEncoding)
{
    std::string urlString = "/foo";
    EXPECT_EQ(nlohmann::json(boost::urls::url(urlString)), urlString);
    EXPECT_EQ(nlohmann::json(boost::urls::url_view(urlString)), urlString);
}

TEST(AppendUrlFromPieces, PiecesAreAppendedViaDelimiters)
{
    boost::urls::url url("/redfish/v1/foo");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/redfish/v1/foo");

    appendUrlPieces(url, "bar");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/redfish/v1/foo/bar");

    appendUrlPieces(url, "/", "bad&string");
    EXPECT_EQ(std::string_view(url.data(), url.size()),
              "/redfish/v1/foo/bar/%2F/bad&string");
}

} // namespace
} // namespace bmcweb::utility
