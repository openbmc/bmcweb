// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "duplicatable_file_handle.hpp"
#include "http_body.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/file_base.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/none.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <format>
#include <span>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

namespace bmcweb
{
namespace
{

TEST(HttpHttpBodyValueType, MoveString)
{
    HttpBody::value_type value("teststring");
    // Move constructor
    HttpBody::value_type value2(std::move(value));
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, MoveOperatorString)
{
    HttpBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    HttpBody::value_type value2 = std::move(value);
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, copysignl)
{
    HttpBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    HttpBody::value_type value2(value);
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, CopyOperatorString)
{
    HttpBody::value_type value;
    value.str() = "teststring";
    // Move constructor
    HttpBody::value_type value2 = value;
    EXPECT_EQ(value2.encodingType, EncodingType::Raw);
    EXPECT_EQ(value2.compressionType, CompressionType::Raw);
    EXPECT_EQ(value2.str(), "teststring");
    EXPECT_EQ(value2.payloadSize(), 10);
}

TEST(HttpHttpBodyValueType, MoveFile)
{
    HttpBody::value_type value(EncodingType::Base64, CompressionType::Raw);
    DuplicatableFileHandle temporaryFile("teststring");
    boost::system::error_code ec;
    value.open(temporaryFile.filePath.c_str(), boost::beast::file_mode::read,
               ec);
    ASSERT_FALSE(ec);
    // Move constructor
    HttpBody::value_type value2(std::move(value));
    std::array<char, 11> buffer{};
    size_t out = value2.file().read(buffer.data(), buffer.size(), ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(value2.encodingType, EncodingType::Base64);
    EXPECT_EQ(value2.compressionType, CompressionType::Raw);

    EXPECT_THAT(std::span(buffer.data(), out),
                ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n', 'g'));

    EXPECT_THAT(buffer, ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n',
                                    'g', '\0'));

    EXPECT_EQ(value2.payloadSize(), 16);
}

TEST(HttpHttpBodyValueType, MoveOperatorFile)
{
    HttpBody::value_type value(EncodingType::Base64, CompressionType::Raw);
    DuplicatableFileHandle temporaryFile("teststring");
    boost::system::error_code ec;
    value.open(temporaryFile.filePath.c_str(), boost::beast::file_mode::read,
               ec);
    ASSERT_FALSE(ec);
    // Move constructor
    HttpBody::value_type value2 = std::move(value);
    std::array<char, 11> buffer{};
    size_t out = value2.file().read(buffer.data(), buffer.size(), ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(value2.encodingType, EncodingType::Base64);
    EXPECT_EQ(value2.compressionType, CompressionType::Raw);

    EXPECT_THAT(std::span(buffer.data(), out),
                ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n', 'g'));
    EXPECT_THAT(buffer, ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n',
                                    'g', '\0'));

    EXPECT_EQ(value2.payloadSize(), 16);
}

TEST(HttpFileBodyValueType, SetFd)
{
    HttpBody::value_type value(EncodingType::Base64, CompressionType::Raw);
    DuplicatableFileHandle temporaryFile("teststring");
    boost::system::error_code ec;
    FILE* r = fopen(temporaryFile.filePath.c_str(), "r");
    ASSERT_NE(r, nullptr);
    value.setFd(fileno(r), ec);
    ASSERT_FALSE(ec);

    std::array<char, 4096> buffer{};

    size_t out = value.file().read(buffer.data(), buffer.size(), ec);
    ASSERT_FALSE(ec);

    EXPECT_THAT(std::span(buffer.data(), out),
                ElementsAre('t', 'e', 's', 't', 's', 't', 'r', 'i', 'n', 'g'));
    EXPECT_EQ(value.payloadSize(), 16);
}

TEST(HttpHttpBodyReader, AccumulatedSizeLimitEnforcedWithoutContentLength)
{
    // HTTP/2 streams DATA frames without a Content-Length: the reader
    // must enforce the body limit on the accumulated size of every put
    // call, not only on a declared length.
    boost::beast::http::request_header<boost::beast::http::fields> headers;
    headers.set(boost::beast::http::field::content_type,
                "application/octet-stream");
    constexpr size_t maxBodySize = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;
    HttpBody::value_type value;
    HttpBody::reader reader(headers, value, maxBodySize);

    boost::beast::error_code initEc;
    reader.init(boost::none, initEc);
    ASSERT_FALSE(initEc);

    // Feed the limit in two puts: the first fits, the second overflows.
    std::string first(maxBodySize / 2, 'a');
    boost::beast::error_code ec;
    size_t out =
        reader.put(boost::asio::const_buffer(first.data(), first.size()), ec);
    ASSERT_FALSE(ec);
    EXPECT_EQ(out, first.size());

    std::string second(maxBodySize, 'b');
    out =
        reader.put(boost::asio::const_buffer(second.data(), second.size()), ec);
    EXPECT_EQ(ec, boost::beast::http::error::body_limit);
    EXPECT_EQ(out, 0);
    EXPECT_EQ(value.str().size(), first.size());
}

TEST(HttpHttpBodyReader, AccumulatedSizeAllowsExactLimit)
{
    boost::beast::http::request_header<boost::beast::http::fields> headers;
    headers.set(boost::beast::http::field::content_type,
                "application/octet-stream");
    constexpr size_t maxBodySize = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;
    HttpBody::value_type value;
    HttpBody::reader reader(headers, value, maxBodySize);

    boost::beast::error_code initEc;
    reader.init(boost::none, initEc);
    ASSERT_FALSE(initEc);

    std::string chunk(maxBodySize / 8, 'a');
    for (size_t i = 0; i < 8; ++i)
    {
        boost::beast::error_code ec;
        size_t out = reader.put(
            boost::asio::const_buffer(chunk.data(), chunk.size()), ec);
        ASSERT_FALSE(ec) << "iteration " << i;
        EXPECT_EQ(out, chunk.size());
    }
    EXPECT_EQ(value.str().size(), maxBodySize);
}

// http_client reads satellite responses through Beast's parser, whose
// body_limit (the aggregator allows 50 MB) bounds them instead of
// BMCWEB_HTTP_BODY_LIMIT.
TEST(HttpHttpBodyReader, ParserLimitBoundsChunkedResponse)
{
    constexpr size_t maxBodySize = 1024UL * 1024UL * BMCWEB_HTTP_BODY_LIMIT;
    constexpr size_t bodySize = maxBodySize + 1;
    std::string message = std::format(
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n{:x}\r\n",
        bodySize);
    message.append(bodySize, 'a');
    message += "\r\n0\r\n\r\n";

    boost::beast::http::response_parser<HttpBody> parser;
    parser.body_limit(2 * maxBodySize);
    parser.eager(true);

    boost::asio::const_buffer input(message.data(), message.size());
    while (!parser.is_done())
    {
        boost::beast::error_code ec;
        size_t used = parser.put(input, ec);
        ASSERT_FALSE(ec) << ec.message();
        ASSERT_NE(used, 0);
        input += used;
    }
    EXPECT_EQ(parser.get().body().str().size(), bodySize);
}

} // namespace
} // namespace bmcweb
