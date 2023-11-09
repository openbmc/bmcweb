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

std::string makeFile(const std::function<std::string()>& sampleData)
{
    std::filesystem::path path = std::filesystem::temp_directory_path();
    path /= "bmcweb_http_response_test_XXXXXXXXXXX";
    std::string stringPath = path.string();
    int fd = mkstemp(stringPath.data());
    EXPECT_GT(fd, 0);
    std::string sample = sampleData();
    EXPECT_EQ(write(fd, sample.data(), sample.size()), sample.size());
    close(fd);
    return stringPath;
}
std::string makeFile()
{
    return makeFile([]() { return "sample text"; });
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
std::string readBody(boost::beast::http::serializer<false, Body>& sr,
                     boost::beast::error_code& ec)
{
    std::string ret;
    while (!sr.is_done())
    {
        sr.next(ec, [&sr, &ret](boost::beast::error_code&, const auto& buffer) {
            std::accumulate(boost::asio::buffer_sequence_begin(buffer),
                            boost::asio::buffer_sequence_end(buffer),
                            std::ref(ret),
                            [&](auto sofar, const auto& innerBuf) {
                auto view = std::string_view(
                    static_cast<char const*>(innerBuf.data()), innerBuf.size());
                sofar.get() += view;
                sr.consume(innerBuf.size());
                return sofar;
            });
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
    std::string path = makeFile();
    res.openFile(path);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
TEST(HttpResponse, FileBodyWithFd)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile();
    int fd = open(path.c_str(), O_RDONLY);
    if (fd != -1)
    {
        res.openFile(fd);
        verifyHeaders(res);
    }
    std::filesystem::remove(path);
}
TEST(HttpResponse, Base64FileBody)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile();
    res.openBase64File(path);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
TEST(HttpResponse, Base64FileBodyWithFd)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile();
    int fd = open(path.c_str(), O_RDONLY);
    if (fd != -1)
    {
        res.openBase64File(path);
        verifyHeaders(res);
    }
    std::filesystem::remove(path);
}
TEST(HttpResponse, BodyTransitions)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makeFile();
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
    std::string path = makeFile([&data]() { return data; });
    FILE* f = fopen(path.c_str(), "r+");
    res.openBase64File(fileno(f));
    testFileData(res, crow::utility::base64encode(data));
    fclose(f);
    std::filesystem::remove(path);
}
TEST(HttpResponse, Base64FileBodyWriterLarge)
{
    crow::Response res;
    auto dataGen = []() {
        std::string result;
        size_t i = 0;
        while (i < 10000)
        {
            result += "sample text";
            i += std::string("sample text").length();
        }
        return result;
    };

    std::string path = makeFile(dataGen);
    FILE* f = fopen(path.c_str(), "r+");
    res.openBase64File(fileno(f));
    auto data = dataGen();
    testFileData(res, crow::utility::base64encode(data));
    fclose(f);
    std::filesystem::remove(path);
}

TEST(HttpResponse, FileBodyWriterLarge)
{
    crow::Response res;
    auto dataGen = []() {
        std::string result;
        size_t i = 0;
        while (i < 10000)
        {
            result += "sample text";
            i += std::string("sample text").length();
        }
        return result;
    };

    std::string path = makeFile(dataGen);
    FILE* f = fopen(path.c_str(), "r+");
    res.openFile(fileno(f));
    testFileData(res, dataGen());
    fclose(f);
    std::filesystem::remove(path);
}

} // namespace
