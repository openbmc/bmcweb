#pragma once
// #include "root_certificates.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
struct SimpleClient
{
    std::string host;
    std::string port;
    using SimpleResponse = http::response<http::string_body>;

    SimpleClient(const char* t, const char* p) : host(t), port(p) {}
    SimpleResponse get(std::string_view target) const
    {
        try
        {
            int version = 11;
            // The SSL context is required, and holds certificates
            ssl::context ctx(ssl::context::tlsv12_client);
            //   load_root_certificates(ctx);

            // Verify the remote server's certificate
            ctx.set_verify_mode(ssl::verify_none);

            // The io_context is required for all I/O
            net::io_context ioc;

            // These objects perform our I/O
            tcp::resolver resolver(ioc);
            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
            // Look up the domain name
            const auto results = resolver.resolve(host, port);

            // Make the connection on the IP address we get from a lookup
            beast::get_lowest_layer(stream).connect(results);
            stream.handshake(ssl::stream_base::client);
            // Set up an HTTP GET request message
            http::request<http::string_body> req{http::verb::get, target,
                                                 version};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            // Send the HTTP request to the remote host
            http::write(stream, req);

            // This buffer is used for reading and must be persisted
            beast::flat_buffer buffer;

            // Declare a container to hold the response
            http::response<http::string_body> res;

            // Receive the HTTP response
            http::read(stream, buffer, res);
            beast::error_code ec;
            stream.shutdown(ec);
            return res;
        }
        catch (std::exception& ex)
        {
            http::response<http::string_body> res;
            res.body() = ex.what();
            return res;
        }
    }
};
