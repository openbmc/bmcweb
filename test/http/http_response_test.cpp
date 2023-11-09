#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "http/http_response.hpp"

#include <filesystem>
#include <fstream>
#include <thread>

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

std::string makeFile(std::string_view sampleData)
{
    std::filesystem::path path = std::filesystem::temp_directory_path();
    path /= "bmcweb_http_response_test_XXXXXXXXXXX";
    std::string stringPath = path.string();
    int fd = mkstemp(stringPath.data());
    EXPECT_GT(fd, 0);
    EXPECT_EQ(write(fd, sampleData.data(), sampleData.size()),
              sampleData.size());
    close(fd);
    return stringPath;
}

void readHeader(boost::beast::http::serializer<false, bmcweb::FileBody>& sr)
{
    while (!sr.is_header_done())
    {
        boost::system::error_code ec;
        sr.next(ec, [&sr](const boost::system::error_code& ec2,
                          const auto& buffer) {
            ASSERT_FALSE(ec2);
            sr.consume(boost::beast::buffer_bytes(buffer));
        });
        ASSERT_FALSE(ec);
    }
}

std::string collectFromBuffers(
    const auto& buffer,
    boost::beast::http::serializer<false, bmcweb::FileBody>& sr)
{
    std::string ret;

    for (auto iter = boost::asio::buffer_sequence_begin(buffer);
         iter != boost::asio::buffer_sequence_end(buffer); ++iter)
    {
        const auto& innerBuf = *iter;
        auto view = std::string_view(static_cast<const char*>(innerBuf.data()),
                                     innerBuf.size());
        ret += view;
        sr.consume(innerBuf.size());
    }
    return ret;
}

std::string
    readBody(boost::beast::http::serializer<false, bmcweb::FileBody>& sr)
{
    std::string ret;
    while (!sr.is_done())
    {
        boost::system::error_code ec;
        sr.next(ec, [&sr, &ret](const boost::system::error_code& ec2,
                                const auto& buffer) {
            ASSERT_FALSE(ec2);
            ret += collectFromBuffers(buffer, sr);
        });
        EXPECT_FALSE(ec);
    }

    return ret;
}
std::string getData(crow::Response::file_response& m)
{
    boost::beast::http::serializer<false, bmcweb::FileBody> sr{m};
    std::stringstream ret;
    sr.split(true);
    readHeader(sr);
    return readBody(sr);
}
TEST(HttpResponse, Defaults)
{
    crow::Response res;
    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::string_response>(
            res.response),
        true);
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
    std::string_view bodyvalue = "this is my new body";
    res.write({bodyvalue.data(), bodyvalue.length()});
    EXPECT_EQ(*res.body(), bodyvalue);
    verifyHeaders(res);
}
TEST(HttpResponse, FileBody)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile("sample text");
    res.openFile(path);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
TEST(HttpResponse, FileBodyWithFd)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile("sample text");
    FILE* fd = fopen(path.c_str(), "r+");
    res.openFd(fileno(fd));
    verifyHeaders(res);
    fclose(fd);
    std::filesystem::remove(path);
}

TEST(HttpResponse, Base64FileBodyWithFd)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile("sample text");
    FILE* fd = fopen(path.c_str(), "r+");
    res.openFd(fileno(fd), bmcweb::EncodingType::Base64);
    verifyHeaders(res);
    fclose(fd);
    std::filesystem::remove(path);
}

TEST(HttpResponse, BodyTransitions)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile("sample text");
    res.openFile(path);

    EXPECT_EQ(boost::variant2::holds_alternative<crow::Response::file_response>(
                  res.response),
              true);

    verifyHeaders(res);
    res.write("body text");

    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::string_response>(
            res.response),
        true);

    verifyHeaders(res);
    std::filesystem::remove(path);
}

void testFileData(crow::Response& res, const std::string& data)
{
    auto& fb =
        boost::variant2::get<crow::Response::file_response>(res.response);
    EXPECT_EQ(getData(fb), data);
}

TEST(HttpResponse, Base64FileBodyWriter)
{
    crow::Response res;
    std::string data = "sample text";
    std::string path = makeFile(data);
    FILE* f = fopen(path.c_str(), "r+");
    res.openFd(fileno(f), bmcweb::EncodingType::Base64);
    testFileData(res, crow::utility::base64encode(data));
    fclose(f);
    std::filesystem::remove(path);
}

static std::string generateBigdata()
{
    std::string result;
    while (result.size() < 10000)
    {
        result += "sample text";
    }
    return result;
}

TEST(HttpResponse, Base64FileBodyWriterLarge)
{
    crow::Response res;
    std::string data = generateBigdata();
    std::string path = makeFile(data);
    {
        boost::beast::file_posix file;
        boost::system::error_code ec;
        file.open(path.c_str(), boost::beast::file_mode::read, ec);
        EXPECT_EQ(ec.value(), 0);
        res.openFd(file.native_handle(), bmcweb::EncodingType::Base64);
        testFileData(res, crow::utility::base64encode(data));
    }

    std::filesystem::remove(path);
}

TEST(HttpResponse, FileBodyWriterLarge)
{
    crow::Response res;
    std::string data = generateBigdata();
    std::string path = makeFile(data);
    {
        boost::beast::file_posix file;
        boost::system::error_code ec;
        file.open(path.c_str(), boost::beast::file_mode::read, ec);
        EXPECT_EQ(ec.value(), 0);
        res.openFd(file.native_handle());
        testFileData(res, data);
    }
    std::filesystem::remove(path);
}

} // namespace
