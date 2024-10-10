#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

namespace bmcweb
{

/*
A test class that simulates a socket by wrapping the beast test stream

Additionally it adds remote_endpoint to allow testing of TCP-specific behaviors
*/
struct TestStream : public boost::beast::test::stream
{
    explicit TestStream(boost::asio::io_context& io) :
        boost::beast::test::stream(io)
    {}

    // NOLINTNEXTLINE(readability-identifier-naming)
    static boost::asio::ip::tcp::endpoint
        remote_endpoint(boost::system::error_code& ec)
    {
        ec = {};
        return {};
    }
};

} // namespace bmcweb

namespace crow = bmcweb;
