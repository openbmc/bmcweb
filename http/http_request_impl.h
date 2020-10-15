#pragma once

#include "common.h"
#include "http/http_request.h"

#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/url/url_view.hpp>

namespace crow
{

struct RequestImpl : Request
{
    boost::beast::http::request<boost::beast::http::string_body>& req;

    std::string userRole{};
    RequestImpl(
        boost::beast::http::request<boost::beast::http::string_body>& reqIn) :
        req(reqIn)
    {}

    std::string& body() const override
    {
        return req.body();
    };

    boost::beast::http::verb method() const override
    {
        return req.method();
    }

    std::string_view getHeaderValue(std::string_view key) const override
    {
        return req[key];
    }

    std::string_view
        getHeaderValue(boost::beast::http::field key) const override
    {
        return req[key];
    }

    std::string_view methodString() const override
    {
        return req.method_string();
    }

    std::string_view target() const override
    {
        return req.target();
    }

    unsigned version() const override
    {
        return req.version();
    }

    bool isUpgrade() const override
    {
        return boost::beast::websocket::is_upgrade(req);
    }

    bool keepAlive() const override
    {
        return req.keep_alive();
    }
};

} // namespace crow
