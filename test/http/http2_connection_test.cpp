#include "http/http2_connection.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
namespace crow
{

namespace
{

using ::testing::Pair;
using ::testing::UnorderedElementsAre;

struct FakeHandler
{
    bool called = false;
    void handle(Request& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        called = true;
        EXPECT_EQ(req.url().buffer(), "/redfish/v1/");
        EXPECT_EQ(req.methodString(), "GET");
        EXPECT_EQ(req.getHeaderValue(boost::beast::http::field::user_agent),
                  "curl/8.5.0");
        EXPECT_EQ(req.getHeaderValue(boost::beast::http::field::accept), "*/*");
        EXPECT_EQ(req.getHeaderValue(":authority"), "localhost:18080");
        asyncResp->res.write("StringOutput");
    }
};

std::string getDateStr()
{
    return "TestTime";
}

void unpackHeaders(std::string_view dataField,
                   std::vector<std::pair<std::string, std::string>>& headers)
{
    nghttp2_hd_inflater inflater;

    while (!dataField.empty())
    {
        nghttp2_nv nv;
        int inflateFlags = 0;
        const uint8_t* data = std::bit_cast<const uint8_t*>(dataField.data());
        ssize_t parsed = inflater.hd2(&nv, &inflateFlags, data,
                                      dataField.size(), 1);

        ASSERT_GT(parsed, 0);
        dataField.remove_prefix(static_cast<size_t>(parsed));
        if (inflateFlags & NGHTTP2_HD_INFLATE_EMIT)
        {
            const char* namePtr = std::bit_cast<const char*>(nv.name);
            std::string key(namePtr, nv.namelen);
            const char* valPtr = std::bit_cast<const char*>(nv.value);
            std::string value(valPtr, nv.valuelen);
            headers.emplace_back(std::make_pair(key, value));
        }
        if (inflateFlags & NGHTTP2_HD_INFLATE_FINAL)
        {
            EXPECT_EQ(inflater.endHeaders(), 0);
            break;
        }
    }
    EXPECT_TRUE(dataField.empty());
}

TEST(http_connection, RequestPropogates)
{
    using namespace std::literals;
    boost::asio::io_context io;
    boost::beast::test::stream stream(io);
    boost::beast::test::stream out(io);
    stream.connect(out);
    // This is a binary pre-encrypted stream captured from curl for a request to
    // curl https://localhost:18080/redfish/v1/
    std::string_view toSend =
        // Hello
        "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
        // 18 byte settings frame
        "\x00\x00\x12\x04\x00\x00\x00\x00\x00"
        // Settings
        "\x00\x03\x00\x00\x00\x64\x00\x04\x00\xa0\x00\x00\x00\x02\x00\x00\x00\x00"
        // Window update frame
        "\x00\x00\x04\x08\x00\x00\x00\x00\x00"
        // Window update
        "\x3e\x7f\x00\x01"
        // Header frame END_STREAM, END_HEADERS set
        "\x00\x00\x29\x01\x05\x00\x00\x00"
        // Header payload
        "\x01\x82\x87\x41\x8b\xa0\xe4\x1d\x13\x9d\x09\xb8\x17\x80\xf0\x3f"
        "\x04\x89\x62\xc2\xc9\x29\x91\x3b\x1d\xc2\xc7\x7a\x88\x25\xb6\x50"
        "\xc3\xcb\xb6\xb8\x3f\x53\x03\x2a\x2f\x2a"sv;

    boost::asio::write(out, boost::asio::buffer(toSend));

    FakeHandler handler;
    boost::asio::steady_timer timer(io);
    std::function<std::string()> date(getDateStr);
    auto conn = std::make_shared<
        HTTP2Connection<boost::beast::test::stream, FakeHandler>>(
        std::move(stream), &handler, date);
    conn->start();

    std::string_view expectedPrefix =
        // Settings frame size 13
        "\x00\x00\x0c\x04\x00\x00\x00\x00\x00"
        // 4 max concurrent streams
        "\x00\x03\x00\x00\x00\x04"
        // Enable push = false
        "\x00\x02\x00\x00\x00\x00"
        // Settings ACK from server to client
        "\x00\x00\x00\x04\x01\x00\x00\x00\x00"

        // Start Headers frame stream 1, size 0x0346
        "\x00\x03\x46\x01\x04\x00\x00\x00\x01"sv;

    std::string_view expectedPostfix =
        // Data Frame, Length 12, Stream 1, End Stream flag set
        "\x00\x00\x0c\x00\x01\x00\x00\x00\x01"
        // The body expected
        "StringOutput"sv;

    std::string_view outStr;
    constexpr size_t headerSize = 0x346;

    while (outStr.size() <
           expectedPrefix.size() + headerSize + expectedPostfix.size())
    {
        io.run_one();
        outStr = out.str();
    }
    EXPECT_TRUE(handler.called);

    EXPECT_TRUE(outStr.starts_with(expectedPrefix));
    outStr.remove_prefix(expectedPrefix.size());
    std::vector<std::pair<std::string, std::string>> headers;
    unpackHeaders(outStr.substr(0, headerSize), headers);
    outStr.remove_prefix(headerSize);

    EXPECT_THAT(
        headers,
        UnorderedElementsAre(
            Pair(":status", "200"),
            Pair("strict-transport-security",
                 "max-age=31536000; includeSubdomains"),
            Pair("x-frame-options", "DENY"), Pair("pragma", "no-cache"),
            Pair("cache-control", "no-store, max-age=0"),
            Pair("x-content-type-options", "nosniff"),
            Pair("referrer-policy", "no-referrer"),
            Pair(
                "permissions-policy",
                "accelerometer=(),ambient-light-sensor=(),autoplay=(),battery=(),camera=(),display-capture=(),document-domain=(),encrypted-media=(),fullscreen=(),gamepad=(),geolocation=(),gyroscope=(),layout-animations=(self),legacy-image-formats=(self),magnetometer=(),microphone=(),midi=(),oversized-images=(self),payment=(),picture-in-picture=(),publickey-credentials-get=(),speaker-selection=(),sync-xhr=(self),unoptimized-images=(self),unsized-media=(self),usb=(),screen-wak-lock=(),web-share=(),xr-spatial-tracking=()"),
            Pair("x-permitted-cross-domain-policies", "none"),
            Pair("cross-origin-embedder-policy", "require-corp"),
            Pair("cross-origin-opener-policy", "same-origin"),
            Pair("cross-origin-resource-policy", "same-origin"),
            Pair(
                "content-security-policy",
                "default-src 'none'; img-src 'self' data:; font-src 'self'; style-src 'self'; script-src 'self'; connect-src 'self' wss:; form-action 'none'; frame-ancestors 'none'; object-src 'none'; base-uri 'none'"),
            Pair("date", "TestTime")));

    EXPECT_EQ(outStr, expectedPostfix);
}

} // namespace
} // namespace crow
