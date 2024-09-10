// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "file_test_utilities.hpp"
#include "http/http_body.hpp"
#include "http/http_response.hpp"
#include "utility.hpp"

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/file_base.hpp>
#include <boost/beast/core/file_posix.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/serializer.hpp>
#include <boost/beast/http/status.hpp>

#include <cstdio>
#include <filesystem>
#include <string>

#include "gtest/gtest.h"
namespace
{
void addHeaders(crow::Response& res)
{
    res.addHeader("myheader", "myvalue");
    res.keepAlive(true);
    res.result(boost::beast::http::status::ok);
}
void verifyHeaders(crow::Response& res)
{
    EXPECT_EQ(res.getHeaderValue("myheader"), "myvalue");
    EXPECT_EQ(res.keepAlive(), true);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
}

std::string getData(boost::beast::http::response<bmcweb::HttpBody>& m)
{
    std::string ret;

    boost::beast::http::response_serializer<bmcweb::HttpBody> sr{m};
    sr.split(true);
    // Reads buffers into ret
    auto reader =
        [&sr, &ret](const boost::system::error_code& ec2, const auto& buffer) {
            EXPECT_FALSE(ec2);
            std::string ret2 = boost::beast::buffers_to_string(buffer);
            sr.consume(ret2.size());
            ret += ret2;
        };
    boost::system::error_code ec;

    // Read headers
    while (!sr.is_header_done())
    {
        sr.next(ec, reader);
        EXPECT_FALSE(ec);
    }
    ret.clear();

    // Read body
    while (!sr.is_done())
    {
        sr.next(ec, reader);
        EXPECT_FALSE(ec);
    }

    return ret;
}

TEST(HttpResponse, Headers)
{
    crow::Response res;
    addHeaders(res);
    verifyHeaders(res);
}
TEST(HttpResponse, StringBody)
{
    crow::Response res;
    addHeaders(res);
    std::string_view bodyValue = "this is my new body";
    res.write({bodyValue.data(), bodyValue.length()});
    EXPECT_EQ(*res.body(), bodyValue);
    verifyHeaders(res);
}
TEST(HttpResponse, HttpBody)
{
    crow::Response res;
    addHeaders(res);
    TemporaryFileHandle temporaryFile("sample text");
    res.openFile(temporaryFile.stringPath);

    verifyHeaders(res);
}
TEST(HttpResponse, HttpBodyWithFd)
{
    crow::Response res;
    addHeaders(res);
    TemporaryFileHandle temporaryFile("sample text");
    FILE* fd = fopen(temporaryFile.stringPath.c_str(), "r+");
    res.openFd(fileno(fd));
    verifyHeaders(res);
    fclose(fd);
}

TEST(HttpResponse, Base64HttpBodyWithFd)
{
    crow::Response res;
    addHeaders(res);
    TemporaryFileHandle temporaryFile("sample text");
    FILE* fd = fopen(temporaryFile.stringPath.c_str(), "r");
    ASSERT_NE(fd, nullptr);
    res.openFd(fileno(fd), bmcweb::EncodingType::Base64);
    verifyHeaders(res);
    fclose(fd);
}

TEST(HttpResponse, BodyTransitions)
{
    crow::Response res;
    addHeaders(res);
    TemporaryFileHandle temporaryFile("sample text");
    res.openFile(temporaryFile.stringPath);

    verifyHeaders(res);
    res.write("body text");

    verifyHeaders(res);
}

std::string generateBigdata()
{
    std::string result;
    while (result.size() < 10000)
    {
        result += "sample text";
    }
    return result;
}

TEST(HttpResponse, StringBodyWriterLarge)
{
    crow::Response res;
    std::string data = generateBigdata();
    res.write(std::string(data));
    EXPECT_EQ(getData(res.response), data);
}

TEST(HttpResponse, Base64HttpBodyWriter)
{
    crow::Response res;
    std::string data = "sample text";
    TemporaryFileHandle temporaryFile(data);
    FILE* f = fopen(temporaryFile.stringPath.c_str(), "r+");
    res.openFd(fileno(f), bmcweb::EncodingType::Base64);
    EXPECT_EQ(getData(res.response), "c2FtcGxlIHRleHQ=");
}

TEST(HttpResponse, Base64HttpBodyWriterLarge)
{
    crow::Response res;
    std::string data = generateBigdata();
    TemporaryFileHandle temporaryFile(data);

    boost::beast::file_posix file;
    boost::system::error_code ec;
    file.open(temporaryFile.stringPath.c_str(), boost::beast::file_mode::read,
              ec);
    EXPECT_EQ(ec.value(), 0);
    res.openFd(file.native_handle(), bmcweb::EncodingType::Base64);
    EXPECT_EQ(getData(res.response), crow::utility::base64encode(data));
}

TEST(HttpResponse, HttpBodyWriterLarge)
{
    crow::Response res;
    std::string data = generateBigdata();
    TemporaryFileHandle temporaryFile(data);

    boost::beast::file_posix file;
    boost::system::error_code ec;
    file.open(temporaryFile.stringPath.c_str(), boost::beast::file_mode::read,
              ec);
    EXPECT_EQ(ec.value(), 0);
    res.openFd(file.native_handle());
    EXPECT_EQ(getData(res.response), data);
}

} // namespace
