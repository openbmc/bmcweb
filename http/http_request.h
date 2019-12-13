#pragma once

#include "sessions.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include "common.h"
#include "query_string.h"

namespace crow
{

struct Request
{
    boost::beast::http::request<boost::beast::http::string_body>& req;
    std::string_view url{};
    QueryString urlParams{};
    bool isSecure{false};

    const std::string& body;

    void* middlewareContext{};
    boost::asio::io_context* ioService{};

    std::shared_ptr<crow::persistent_data::UserSession> session;

    std::string userRole{};

    Request(
        boost::beast::http::request<boost::beast::http::string_body>& reqIn) :
        req(reqIn),
        body(reqIn.body())
    {
    }

    boost::beast::http::verb method() const
    {
        return req.method();
    }

    const std::string_view getHeaderValue(std::string_view key) const
    {
        return req[key];
    }

    const std::string_view getHeaderValue(boost::beast::http::field key) const
    {
        return req[key];
    }

    const std::string_view methodString() const
    {
        return req.method_string();
    }

    const std::string_view target() const
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
};

} // namespace crow
