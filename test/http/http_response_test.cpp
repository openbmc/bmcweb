#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/http/serializer.hpp"
#include "http/http_connection.hpp"
#include "http/http_response.hpp"

#include <filesystem>
#include <fstream>

#include "gtest/gtest.h"
using namespace crow;
using namespace boost::beast::http;
using namespace boost::asio;
using namespace boost::beast;
void addHeaders(Response& res)
{
    res.addHeader("myheader", "myvalue");
    res.keepAlive(true);
    res.result(boost::beast::http::status::ok);
}
void varifyHeaders(Response& res)
{
    EXPECT_EQ(res.getHeaderValue("myheader"), "myvalue");
    EXPECT_EQ(res.keepAlive(), true);
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
}

template <class Serializer>
struct lambda
{
    Serializer& sr;
    mutable boost::beast::flat_buffer buffer;
    lambda(Serializer& sr_) : sr(sr_) {}

    template <class ConstBufferSequence>
    void operator()(error_code& ec, const ConstBufferSequence& buffers) const
    {
        ec = {};
        buffer.commit(
            net::buffer_copy(buffer.prepare(buffer_bytes(buffers)), buffers));
        sr.consume(buffer_bytes(buffers));
    }
};

template <bool isRequest, class Body, class Fields>
auto writeMessage(message<isRequest, Body, Fields>& m, error_code& ec)
{
    serializer<isRequest, Body, Fields> sr{m};
    sr.split(true);
    auto body = lambda(sr);
    auto header = lambda(sr);
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
    Response res;
    EXPECT_EQ(std::holds_alternative<Response::string_body_response_type>(
                  res.genericResponse.value()),
              true);
}
TEST(http_response, headers)
{
    Response res;
    addHeaders(res);
    varifyHeaders(res);
}
TEST(http_response, stringbody)
{
    Response res;
    addHeaders(res);
    auto& body = res.body();
    std::string_view bodyvalue = "this is my new body";
    body += bodyvalue;
    EXPECT_EQ(res.body(), bodyvalue);
    varifyHeaders(res);
}
TEST(http_response, filebody)
{
    Response res;
    addHeaders(res);
    std::ofstream file;
    std::string_view s = "sample text";
    std::string_view path = "/tmp/temp.txt";
    file.open(path.data());
    file << s;
    file.close();

    res.openFile(path);

    error_code ec{};
    Response::file_body_response_type& body_resp =
        std::get<Response::file_body_response_type>(*res.genericResponse);

    lambda visit = writeMessage(body_resp, ec);

    // EXPECT_EQ(ec, 0);
    const auto b = buffers_front(visit.buffer.data());
    std::string_view s1{reinterpret_cast<const char*>(b.data()), b.size()};
    EXPECT_EQ(s1, s);
    std::filesystem::remove(path);

    varifyHeaders(res);
}
TEST(http_response, body_transitions)
{
    Response res;
    addHeaders(res);
    std::ofstream file;
    std::string_view s = "sample text";
    std::string_view path = "/tmp/temp.txt";
    file.open(path.data());
    file << s;
    file.close();

    res.openFile(path);

    EXPECT_EQ(std::holds_alternative<Response::file_body_response_type>(
                  res.genericResponse.value()),
              true);

    varifyHeaders(res);
    res.body() += "body text";

    EXPECT_EQ(std::holds_alternative<Response::string_body_response_type>(
                  res.genericResponse.value()),
              true);

    varifyHeaders(res);
    std::filesystem::remove(path);
}
