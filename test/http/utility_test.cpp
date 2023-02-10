#include "bmcweb_config.h"

#include "utility.hpp"

#include <boost/url/error.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <ctime>
#include <functional>
#include <limits>
#include <string>
#include <string_view>

#include <gtest/gtest.h> // IWYU pragma: keep
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace crow::utility
{
namespace
{

using ::crow::black_magic::getParameterTag;

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

TEST(Utility, UrlFromPieces)
{
    boost::urls::url url = urlFromPieces("redfish", "v1", "foo");
    EXPECT_EQ(url.buffer(), "/redfish/v1/foo");

    url = urlFromPieces("/", "badString");
    EXPECT_EQ(url.buffer(), "/%2F/badString");

    url = urlFromPieces("bad?tring");
    EXPECT_EQ(url.buffer(), "/bad%3Ftring");

    url = urlFromPieces("/", "bad&tring");
    EXPECT_EQ(url.buffer(), "/%2F/bad&tring");

    EXPECT_EQ(std::string_view(url.data(), url.size()), "/%2F/bad&tring");

    url = urlFromPieces("my-user");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/my-user");

    url = urlFromPieces("my_user");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/my_user");

    url = urlFromPieces("my_93user");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/my_93user");

    // The following characters will be converted to ASCII number
    // `[{]}\|"<>/?#%^
    url =
        urlFromPieces("~1234567890-_=+qwertyuiopasdfghjklzxcvbnm;:',.!@$&*()");
    EXPECT_EQ(std::string_view(url.data(), url.size()),
              "/~1234567890-_=+qwertyuiopasdfghjklzxcvbnm;:',.!@$&*()");
}

TEST(Utility, readUrlSegments)
{
    boost::urls::result<boost::urls::url_view> parsed =
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

TEST(Utility, ValidateAndSplitUrlPositive)
{
    std::string host;
    std::string urlProto;
    uint16_t port = 0;
    std::string path;
    ASSERT_TRUE(validateAndSplitUrl("https://foo.com:18080/bar", urlProto, host,
                                    port, path));
    EXPECT_EQ(host, "foo.com");
    EXPECT_EQ(urlProto, "https");
    EXPECT_EQ(port, 18080);

    EXPECT_EQ(path, "/bar");

    // query string
    ASSERT_TRUE(validateAndSplitUrl("https://foo.com:18080/bar?foobar=1",
                                    urlProto, host, port, path));
    EXPECT_EQ(path, "/bar?foobar=1");

    // fragment
    ASSERT_TRUE(validateAndSplitUrl("https://foo.com:18080/bar#frag", urlProto,
                                    host, port, path));
    EXPECT_EQ(path, "/bar#frag");

    // Missing port
    ASSERT_TRUE(
        validateAndSplitUrl("https://foo.com/bar", urlProto, host, port, path));
    EXPECT_EQ(port, 443);

    // Missing path defaults to "/"
    ASSERT_TRUE(
        validateAndSplitUrl("https://foo.com/", urlProto, host, port, path));
    EXPECT_EQ(path, "/");

    // If http push eventing is allowed, allow http and pick a default port of
    // 80, if it's not, parse should fail.
    ASSERT_EQ(
        validateAndSplitUrl("http://foo.com/bar", urlProto, host, port, path),
        bmcwebInsecureEnableHttpPushStyleEventing);
    if constexpr (bmcwebInsecureEnableHttpPushStyleEventing)
    {
        EXPECT_EQ(port, 80);
    }
}

TEST(Router, ParameterTagging)
{
    EXPECT_EQ(1, getParameterTag("<str>"));
    EXPECT_EQ(1, getParameterTag("<string>"));
    EXPECT_EQ(2, getParameterTag("<path>"));
}

TEST(URL, JsonEncoding)
{
    std::string urlString = "/foo";
    EXPECT_EQ(nlohmann::json(boost::urls::url(urlString)), urlString);
    EXPECT_EQ(nlohmann::json(boost::urls::url_view(urlString)), urlString);
}

TEST(AppendUrlFromPieces, PiecesAreAppendedViaDelimiters)
{
    boost::urls::url url = urlFromPieces("redfish", "v1", "foo");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/redfish/v1/foo");

    appendUrlPieces(url, "bar");
    EXPECT_EQ(std::string_view(url.data(), url.size()), "/redfish/v1/foo/bar");

    appendUrlPieces(url, "/", "bad&tring");
    EXPECT_EQ(std::string_view(url.data(), url.size()),
              "/redfish/v1/foo/bar/%2F/bad&tring");
}

} // namespace
} // namespace crow::utility
