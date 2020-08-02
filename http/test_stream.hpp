#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

namespace crow
{

/*
A test class that simulates a socket by wrapping the beast test stream

Additionally it adds remote_endpoint to allow testing of TCP-specific behaviors
*/
struct TestStream : public boost::beast::test::stream
{
    TestStream(boost::asio::io_context& io) : boost::beast::test::stream(io) {}

    boost::asio::ip::tcp::endpoint
        remote_endpoint(boost::system::error_code& ec)
    {
        ec = {};
        return {};
    }
};

} // namespace crow
