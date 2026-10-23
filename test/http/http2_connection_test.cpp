// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "http/http2_connection.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http_connect_types.hpp"
#include "nghttp2_adapters.hpp"
#include "test_stream.hpp"

#include <nghttp2/nghttp2.h>
#include <sys/types.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/http/field.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
    void handle(const std::shared_ptr<Request>& req,
                const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        called = true;
        EXPECT_EQ(req->url().buffer(), "/redfish/v1/");
        EXPECT_EQ(req->methodString(), "GET");
        EXPECT_EQ(req->getHeaderValue(boost::beast::http::field::user_agent),
                  "curl/8.5.0");
        EXPECT_EQ(req->getHeaderValue(boost::beast::http::field::accept),
                  "*/*");
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
    nghttp2_hd_inflater_ex inflater;

    while (!dataField.empty())
    {
        nghttp2_nv nv;
        int inflateFlags = 0;
        const uint8_t* data = std::bit_cast<const uint8_t*>(dataField.data());
        ssize_t parsed =
            inflater.hd2(&nv, &inflateFlags, data, dataField.size(), 1);

        ASSERT_GT(parsed, 0);
        dataField.remove_prefix(static_cast<size_t>(parsed));
        if ((inflateFlags & NGHTTP2_HD_INFLATE_EMIT) > 0)
        {
            const char* namePtr = std::bit_cast<const char*>(nv.name);
            std::string key(namePtr, nv.namelen);
            const char* valPtr = std::bit_cast<const char*>(nv.value);
            std::string value(valPtr, nv.valuelen);
            headers.emplace_back(key, value);
        }
        if ((inflateFlags & NGHTTP2_HD_INFLATE_FINAL) > 0)
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
    TestStream stream(io);
    TestStream out(io);
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
    boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tls_server);
    auto conn = std::make_shared<HTTP2Connection<TestStream, FakeHandler>>(
        boost::asio::ssl::stream<TestStream>(std::move(stream), sslCtx),
        &handler, date, HttpType::HTTP, nullptr, boost::asio::ip::address());
    conn->start();

    std::array<std::string_view, 9> expectedPrefix = {
        // Settings frame size 24
        "\x00\x00\x18\x04\x00\x00\x00\x00\x00"sv,
        // 4 max concurrent streams
        "\x00\x03\x00\x00\x00\x04"sv,
        // Enable push = false
        "\x00\x02\x00\x00\x00\x00"sv,
        // Max window size 1 << 20
        "\x00\x04\x00\x10\x00\x00"sv,
        // Max frame size 1 << 14
        "\x00\x05\x00\x00\x40\x00"sv,

        // Frame window update stream 0
        "\x00\x00\x04\x08\x00\x00\x00\x00\x00\x00\x0f\x00\x01"sv,

        // Settings ACK from server to client
        "\x00\x00\x00\x04\x01\x00\x00\x00\x00"sv,

        // Window update stream 1
        "\x00\x00\x04\x08\x00\x00\x00\x00\x01\x00\x07\x00\x01"sv,

        // Start Headers frame stream 1, size 0x005f
        "\x00\x00\x5f\x01\x04\x00\x00\x00\x01"sv,
    };

    std::string_view expectedPostfix =
        // Data Frame, Length 12, Stream 1, End Stream flag set
        "\x00\x00\x0c\x00\x01\x00\x00\x00\x01"
        // The body expected
        "StringOutput"sv;

    std::string_view outStr;
    constexpr size_t headerSize = 0x05f;

    size_t expectedPrefixSize = 0;
    for (const auto prefix : expectedPrefix)
    {
        expectedPrefixSize += prefix.size();
    }

    // Run until we receive the expected amount of data
    while (outStr.size() <
           expectedPrefixSize + headerSize + expectedPostfix.size())
    {
        io.run_one();
        outStr = out.str();
    }
    EXPECT_TRUE(handler.called);

    // check the stream output against expected
    for (const auto& prefix : expectedPrefix)
    {
        EXPECT_EQ(outStr.substr(0, prefix.size()), prefix);
        outStr.remove_prefix(prefix.size());
    }

    std::vector<std::pair<std::string, std::string>> headers;
    unpackHeaders(outStr.substr(0, headerSize), headers);
    outStr.remove_prefix(headerSize);

    EXPECT_THAT(headers,
                UnorderedElementsAre(
                    Pair(":status", "200"), Pair("content-length", "12"),
                    Pair("strict-transport-security",
                         "max-age=31536000; includeSubdomains"),
                    Pair("cache-control", "no-store, max-age=0"),
                    Pair("x-content-type-options", "nosniff"),
                    Pair("pragma", "no-cache"), Pair("date", "TestTime")));

    EXPECT_EQ(outStr, expectedPostfix);
}

void getRstStreamError(nghttp2_session& session, int32_t streamId,
                       std::optional<uint32_t>& errorCode)
{
    while (true)
    {
        std::span<const uint8_t> output = session.memSend();
        if (output.empty())
        {
            return;
        }

        size_t offset = 0;
        while (offset < output.size())
        {
            ASSERT_GE(output.size() - offset, 9);
            size_t frameLength =
                (static_cast<size_t>(output[offset]) << 16U) |
                (static_cast<size_t>(output[offset + 1]) << 8U) |
                static_cast<size_t>(output[offset + 2]);
            ASSERT_GE(output.size() - offset, 9 + frameLength);
            uint8_t frameType = output[offset + 3];
            uint32_t frameStreamId =
                (static_cast<uint32_t>(output[offset + 5] & 0x7FU) << 24U) |
                (static_cast<uint32_t>(output[offset + 6]) << 16U) |
                (static_cast<uint32_t>(output[offset + 7]) << 8U) |
                static_cast<uint32_t>(output[offset + 8]);
            if (frameType == NGHTTP2_RST_STREAM &&
                frameStreamId == static_cast<uint32_t>(streamId))
            {
                ASSERT_EQ(frameLength, 4);
                errorCode =
                    (static_cast<uint32_t>(output[offset + 9]) << 24U) |
                    (static_cast<uint32_t>(output[offset + 10]) << 16U) |
                    (static_cast<uint32_t>(output[offset + 11]) << 8U) |
                    static_cast<uint32_t>(output[offset + 12]);
            }
            offset += 9 + frameLength;
        }
    }
}

TEST(http_connection, BodyLimitResetsOnlyTheStream)
{
    using namespace std::literals;
    boost::asio::io_context io;
    TestStream stream(io);
    TestStream out(io);
    stream.connect(out);

    FakeHandler handler;
    std::function<std::string()> date(getDateStr);
    boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tls_server);
    auto conn = std::make_shared<HTTP2Connection<TestStream, FakeHandler>>(
        boost::asio::ssl::stream<TestStream>(std::move(stream), sslCtx),
        &handler, date, HttpType::HTTP, nullptr, boost::asio::ip::address());

    std::string_view requestHeaders =
        "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
        "\x00\x00\x00\x04\x00\x00\x00\x00\x00"
        "\x00\x00\x29\x01\x04\x00\x00\x00\x01"
        "\x82\x87\x41\x8b\xa0\xe4\x1d\x13\x9d\x09\xb8\x17\x80\xf0\x3f"
        "\x04\x89\x62\xc2\xc9\x29\x91\x3b\x1d\xc2\xc7\x7a\x88\x25\xb6\x50"
        "\xc3\xcb\xb6\xb8\x3f\x53\x03\x2a\x2f\x2a"sv;
    std::span<const uint8_t> input(
        std::bit_cast<const uint8_t*>(requestHeaders.data()),
        requestHeaders.size());
    ASSERT_EQ(conn->ngSession.memRecv(input),
              static_cast<ssize_t>(input.size()));
    ASSERT_TRUE(conn->streams.contains(1));

    constexpr size_t maxBodySize = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;
    std::vector<uint8_t> body(maxBodySize, 'a');
    EXPECT_EQ(conn->onDataChunkRecvCallback(0, 1, body.data(), body.size()), 0);

    constexpr uint8_t extra = 'b';
    EXPECT_EQ(conn->onDataChunkRecvCallback(0, 1, &extra, 1), 0);
    auto streamIt = conn->streams.find(1);
    ASSERT_NE(streamIt, conn->streams.end());
    EXPECT_TRUE(streamIt->second.bodyLimitExceeded);
    EXPECT_FALSE(streamIt->second.reqReader);
    EXPECT_TRUE(streamIt->second.req->req.body().str().empty());
    EXPECT_LT(streamIt->second.req->req.body().str().capacity(), maxBodySize);

    EXPECT_EQ(conn->onDataChunkRecvCallback(0, 1, &extra, 1), 0);
    EXPECT_TRUE(streamIt->second.req->req.body().str().empty());
    EXPECT_EQ(conn->onRequestRecv(1), 0);
    EXPECT_FALSE(handler.called);

    std::string_view postResetData =
        "\x00\x00\x01\x00\x01\x00\x00\x00\x01\x63"sv;
    std::span<const uint8_t> postResetInput(
        std::bit_cast<const uint8_t*>(postResetData.data()),
        postResetData.size());
    ASSERT_EQ(conn->ngSession.memRecv(postResetInput),
              static_cast<ssize_t>(postResetInput.size()));
    EXPECT_FALSE(handler.called);

    std::optional<uint32_t> resetError;
    getRstStreamError(conn->ngSession, 1, resetError);
    ASSERT_TRUE(resetError);
    EXPECT_EQ(*resetError, NGHTTP2_ENHANCE_YOUR_CALM);

    std::string_view nextRequestHeaders =
        "\x00\x00\x29\x01\x05\x00\x00\x00\x03"
        "\x82\x87\x41\x8b\xa0\xe4\x1d\x13\x9d\x09\xb8\x17\x80\xf0\x3f"
        "\x04\x89\x62\xc2\xc9\x29\x91\x3b\x1d\xc2\xc7\x7a\x88\x25\xb6\x50"
        "\xc3\xcb\xb6\xb8\x3f\x53\x03\x2a\x2f\x2a"sv;
    std::span<const uint8_t> nextInput(
        std::bit_cast<const uint8_t*>(nextRequestHeaders.data()),
        nextRequestHeaders.size());
    ASSERT_EQ(conn->ngSession.memRecv(nextInput),
              static_cast<ssize_t>(nextInput.size()));
    EXPECT_TRUE(handler.called);
}

TEST(http_connection, DeclaredBodyLimitResetsTheStream)
{
    using namespace std::literals;
    boost::asio::io_context io;
    TestStream stream(io);
    TestStream out(io);
    stream.connect(out);

    FakeHandler handler;
    std::function<std::string()> date(getDateStr);
    boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tls_server);
    auto conn = std::make_shared<HTTP2Connection<TestStream, FakeHandler>>(
        boost::asio::ssl::stream<TestStream>(std::move(stream), sslCtx),
        &handler, date, HttpType::HTTP, nullptr, boost::asio::ip::address());

    std::string_view requestHeaders =
        "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
        "\x00\x00\x00\x04\x00\x00\x00\x00\x00"
        "\x00\x00\x29\x01\x04\x00\x00\x00\x01"
        "\x82\x87\x41\x8b\xa0\xe4\x1d\x13\x9d\x09\xb8\x17\x80\xf0\x3f"
        "\x04\x89\x62\xc2\xc9\x29\x91\x3b\x1d\xc2\xc7\x7a\x88\x25\xb6\x50"
        "\xc3\xcb\xb6\xb8\x3f\x53\x03\x2a\x2f\x2a"sv;
    std::span<const uint8_t> input(
        std::bit_cast<const uint8_t*>(requestHeaders.data()),
        requestHeaders.size());
    ASSERT_EQ(conn->ngSession.memRecv(input),
              static_cast<ssize_t>(input.size()));
    auto streamIt = conn->streams.find(1);
    ASSERT_NE(streamIt, conn->streams.end());

    constexpr size_t maxBodySize = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;
    streamIt->second.contentLength = maxBodySize + 1;

    constexpr uint8_t data = 'a';
    EXPECT_EQ(conn->onDataChunkRecvCallback(0, 1, &data, 1), 0);
    EXPECT_TRUE(streamIt->second.bodyLimitExceeded);
    EXPECT_FALSE(streamIt->second.reqReader);
    EXPECT_TRUE(streamIt->second.req->req.body().str().empty());
    EXPECT_EQ(conn->onRequestRecv(1), 0);
    EXPECT_FALSE(handler.called);

    std::optional<uint32_t> resetError;
    getRstStreamError(conn->ngSession, 1, resetError);
    ASSERT_TRUE(resetError);
    EXPECT_EQ(*resetError, NGHTTP2_ENHANCE_YOUR_CALM);
}

} // namespace
} // namespace crow
