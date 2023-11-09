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

template <typename Body>
void readHeader(boost::beast::http::serializer<false, Body>& sr,
                boost::beast::error_code& ec)
{
    while (!sr.is_header_done())
    {
        sr.next(ec, [&sr](boost::beast::error_code&, const auto& buffer) {
            sr.consume(boost::beast::buffer_bytes(buffer));
        });
    }
}
template <typename Body>
static std::string
    collectFromBuffers(const auto& buffer,
                       boost::beast::http::serializer<false, Body>& sr)
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
template <typename Body>
std::string readBody(boost::beast::http::serializer<false, Body>& sr,
                     boost::beast::error_code& ec)
{
    std::string ret;
    while (!sr.is_done())
    {
        sr.next(ec, [&sr, &ret](boost::beast::error_code&, const auto& buffer) {
            ret += collectFromBuffers(buffer, sr);
        });
    }

    return ret;
}
template <class Body, typename Fields>
std::string getData(boost::beast::http::message<false, Body, Fields>& m,
                    boost::beast::error_code& ec)
{
    boost::beast::http::serializer<false, Body> sr{m};
    std::stringstream ret;
    sr.split(true);
    readHeader(sr, ec);
    if (ec)
    {
        return {};
    }
    return readBody(sr, ec);
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
    res.openFile(fileno(fd));
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
    res.openBase64File(fileno(fd));
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
    boost::variant2::visit(
        [&data](auto&& arg) {
        boost::beast::error_code ec;
        auto fileData = getData(arg, ec);
        EXPECT_EQ(ec.value(), 0);
        EXPECT_EQ(fileData, data);
    },
        res.response);
}
TEST(HttpResponse, Base64FileBodyWriter)
{
    crow::Response res;
    std::string data = "sample text";
    std::string path = makeFile(data);
    FILE* f = fopen(path.c_str(), "r+");
    res.openBase64File(fileno(f));
    testFileData(res, crow::utility::base64encode(data));
    fclose(f);
    std::filesystem::remove(path);
}
static inline std::string generateBigdata()
{
    std::string result;
    size_t i = 0;
    while (i < 10000)
    {
        result += "sample text";
        i += std::string("sample text").length();
    }
    return result;
}
TEST(HttpResponse, Base64FileBodyWriterLarge)
{
    crow::Response res;
    std::string data = generateBigdata();
    std::string path = makeFile(data);
    boost::beast::file_posix file;
    boost::system::error_code ec;
    file.open(path.c_str(), boost::beast::file_mode::read, ec);
    EXPECT_EQ(ec.value(), 0);
    res.openBase64File(file.native_handle());
    testFileData(res, crow::utility::base64encode(data));
    file.close(ec);
    EXPECT_EQ(ec.value(), 0);
    std::filesystem::remove(path);
}

TEST(HttpResponse, FileBodyWriterLarge)
{
    crow::Response res;
    std::string data = generateBigdata();
    std::string path = makeFile(data);
    boost::beast::file_posix file;
    boost::system::error_code ec;
    file.open(path.c_str(), boost::beast::file_mode::read, ec);
    EXPECT_EQ(ec.value(), 0);
    res.openFile(file.native_handle());
    testFileData(res, data);
    file.close(ec);
    EXPECT_EQ(ec.value(), 0);
    std::filesystem::remove(path);
}

} // namespace
