#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "http/http_connection.hpp"
#include "http/http_response.hpp"

#include <filesystem>
#include <fstream>

#include "gtest/gtest.h"

static void addHeaders(crow::Response& res)
{
    res.addHeader("myheader", "myvalue");
    res.keepAlive(true);
    res.result(boost::beast::http::status::ok);
}
static void varifyHeaders(crow::Response& res)
{
    EXPECT_EQ(res.getHeaderValue("myheader"), "myvalue");
    EXPECT_EQ(res.keepAlive(), true);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
}

template <class Serializer>
struct Lambda
{
    Serializer& sr;
    mutable boost::beast::flat_buffer buffer;
    explicit Lambda(Serializer& s) : sr(s) {}

    template <class ConstBufferSequence>
    void operator()(boost::beast::error_code& ec,
                    const ConstBufferSequence& buffers) const
    {
        ec = {};
        buffer.commit(boost::asio::buffer_copy(
            buffer.prepare(boost::beast::buffer_bytes(buffers)), buffers));
        sr.consume(boost::beast::buffer_bytes(buffers));
    }
};
template <class Serializer>
Lambda(Serializer&) -> Lambda<Serializer>;

template <bool isRequest, class Body, class Fields>
auto writeMessage(boost::beast::http::serializer<isRequest, Body, Fields>& sr,
                  boost::beast::error_code& ec)
{
    sr.split(true);
    Lambda body(sr);
    Lambda header(sr);
    do
    {
        sr.next(ec, header);
    } while (!sr.is_header_done());
    if (!ec && !sr.is_done())
    {
        do
        {
            sr.next(ec, body);
        } while (!ec && !sr.is_done());
    }
    return body;
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
    varifyHeaders(res);
}
TEST(http_response, stringbody)
{
    crow::Response res;
    addHeaders(res);
    std::string_view bodyvalue = "this is my new body";
    res.write({bodyvalue.data(), bodyvalue.length()});
    EXPECT_EQ(*res.body(), bodyvalue);
    varifyHeaders(res);
}
TEST(http_response, filebody)
{
    crow::Response res;
    addHeaders(res);
    std::ofstream file;
    std::string_view s = "sample text";
    std::string_view path = "/tmp/temp.txt";
    file.open(path.data());
    file << s;
    file.close();

    res.openFile(path);

    boost::beast::error_code ec{};
    crow::Response::file_response& bodyResp =
        boost::variant2::get<crow::Response::file_response>(res.response);
    boost::beast::http::response_serializer<boost::beast::http::file_body> sr{
        bodyResp};
    Lambda visit = writeMessage(sr, ec);

    // EXPECT_EQ(ec, 0);
    const auto b = boost::beast::buffers_front(visit.buffer.data());
    std::string_view s1{static_cast<char*>(b.data()), b.size()};
    EXPECT_EQ(s1, s);
    std::filesystem::remove(path);

    varifyHeaders(res);
}
TEST(http_response, body_transitions)
{
    crow::Response res;
    addHeaders(res);
    std::ofstream file;
    std::string_view s = "sample text";
    std::string_view path = "/tmp/temp.txt";
    file.open(path.data());
    file << s;
    file.close();

    res.openFile(path);

    EXPECT_EQ(boost::variant2::holds_alternative<crow::Response::file_response>(
                  res.response),
              true);

    varifyHeaders(res);
    res.write("body text");

    EXPECT_EQ(
        boost::variant2::holds_alternative<crow::Response::string_response>(
            res.response),
        true);

    varifyHeaders(res);
    std::filesystem::remove(path);
}
