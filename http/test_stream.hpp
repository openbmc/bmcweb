#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

namespace crow
{

/*
A test class that simulates a socket by wrapping the beast test stream

Additionally it adds remote_endpoint and set_option to allow testing of
TCP-specific behaviors
*/
struct TestStream : public boost::beast::test::stream
{
    explicit TestStream(boost::asio::io_context& io) :
        boost::beast::test::stream(io)
    {}

    using endpoint = boost::asio::ip::tcp::endpoint;
    // NOLINTNEXTLINE(readability-identifier-naming)
    static endpoint remote_endpoint(boost::system::error_code& ec)
    {
        ec = {};
        return {};
    }

    template <typename SettableSocketOption>
    // NOLINTNEXTLINE(readability-identifier-naming)
    void set_option(const SettableSocketOption& opt)
    {
        (void)opt;
    }
};

} // namespace crow
