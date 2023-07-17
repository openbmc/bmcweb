#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "http/http_response.hpp"

#include <filesystem>
#include <fstream>
#include <thread>

#include "gtest/gtest.h"

static void addHeaders(crow::Response& res)
{
    res.addHeader("myheader", "myvalue");
    res.keepAlive(true);
    res.result(boost::beast::http::status::ok);
}
static void verifyHeaders(crow::Response& res)
{
    EXPECT_EQ(res.getHeaderValue("myheader"), "myvalue");
    EXPECT_EQ(res.keepAlive(), true);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
}
static std::string makePath(std::thread::id threadID)
{
    std::stringstream stream;
    stream << "/tmp/" << threadID << ".txt";
    return stream.str();
}
static void makeFile(const std::string& path)
{
    std::ofstream file;
    std::string_view s = "sample text";
    file.open(path);
    file << s;
    file.close();
}
TEST(http_response, Defaults)
{
    crow::Response res;
    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::string_response>(
            res.response),
        true);
}
TEST(http_response, headers)
{
    crow::Response res;
    addHeaders(res);
    verifyHeaders(res);
}
TEST(http_response, stringbody)
{
    crow::Response res;
    addHeaders(res);
    std::string_view bodyvalue = "this is my new body";
    res.write({bodyvalue.data(), bodyvalue.length()});
    EXPECT_EQ(*res.body(), bodyvalue);
    verifyHeaders(res);
}
TEST(http_response, filebody)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makePath(std::this_thread::get_id());
    makeFile(path);
    res.openFile(path);

    verifyHeaders(res);
    std::filesystem::remove(path);
}
TEST(http_response, body_transitions)
{
    crow::Response res;
    addHeaders(res);
    std::string path = makePath(std::this_thread::get_id());
    makeFile(path);
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
