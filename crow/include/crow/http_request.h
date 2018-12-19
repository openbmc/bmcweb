#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include "crow/common.h"
#include "crow/query_string.h"

namespace crow
{

struct Request
{
    boost::string_view url{};
    QueryString urlParams{};
    bool isSecure{false};

    const std::string& body;

    void* middlewareContext{};
    boost::asio::io_context* ioService{};

    Request(boost::beast::http::request<boost::beast::http::string_body>& req) :
        req(req), body(req.body())
    {
    }

    const boost::beast::http::verb method() const
    {
        return req.method();
    }

    const boost::string_view getHeaderValue(boost::string_view key) const
    {
        return req[key];
    }

    const boost::string_view getHeaderValue(boost::beast::http::field key) const
    {
        return req[key];
    }

    const boost::string_view methodString() const
    {
        return req.method_string();
    }

    const boost::string_view target() const
    {
        return req.target();
    }

    unsigned version()
    {
        return req.version();
    }

    bool isUpgrade()
    {
        return boost::beast::websocket::is_upgrade(req);
    }

    bool keepAlive()
    {
        return req.keep_alive();
    }

    boost::beast::http::request<boost::beast::http::string_body>& req;
};

} // namespace crow
