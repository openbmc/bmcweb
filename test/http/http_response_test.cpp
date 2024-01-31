#include "boost/beast/core/buffers_to_string.hpp"
#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "file_test_utilities.hpp"
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

std::string getData(boost::beast::http::response<bmcweb::HttpBody>& m)
{
    std::string ret;

    boost::beast::http::response_serializer<bmcweb::HttpBody> sr{m};
    sr.split(true);

    auto reader = [&sr, &ret](const boost::system::error_code& ec2,
                              const auto& buffer) {
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
    std::string_view bodyvalue = "this is my new body";
    res.write({bodyvalue.data(), bodyvalue.length()});
    EXPECT_EQ(*res.body(), bodyvalue);
    verifyHeaders(res);
}
TEST(HttpResponse, HttpBody)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile("sample text");
    res.openFile(path);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
TEST(HttpResponse, HttpBodyWithFd)
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

TEST(HttpResponse, Base64HttpBodyWithFd)
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

    verifyHeaders(res);
    res.write("body text");

    verifyHeaders(res);
    std::filesystem::remove(path);
}

void testFileData(crow::Response& res, const std::string& data)
{
    auto& fb = res.response;
    EXPECT_EQ(getData(fb), data);
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
    testFileData(res, data);
}

TEST(HttpResponse, Base64HttpBodyWriter)
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

TEST(HttpResponse, Base64HttpBodyWriterLarge)
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

TEST(HttpResponse, HttpBodyWriterLarge)
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
